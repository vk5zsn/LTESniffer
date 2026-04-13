#include "srsran/srsran.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  const char* input_path;
  const char* output_path;
  uint32_t    pci;
  uint32_t    nof_prb;
  uint32_t    nof_subframes;
  uint32_t    max_search_subframes;
} args_t;

typedef struct {
  FILE*    file;
  bool     eof;
  uint64_t samples_read;
} file_stream_t;

static void usage(const char* prog)
{
  fprintf(stderr,
          "Usage: %s -i <input_cf32> -o <output_cf32> -c <pci> -p <prb> [-n <subframes>] [-m <search_subframes>]\n",
          prog);
  fprintf(stderr, "  -i input raw cf32 IQ file\n");
  fprintf(stderr, "  -o output raw cf32 IQ file with synchronized LTE subframes\n");
  fprintf(stderr, "  -c LTE PCI\n");
  fprintf(stderr, "  -p LTE PRB count\n");
  fprintf(stderr, "  -n number of synchronized subframes to write (default: until EOF)\n");
  fprintf(stderr, "  -m maximum raw subframes to search before giving up (default: 4000)\n");
}

static bool parse_args(int argc, char** argv, args_t* args)
{
  int opt;

  memset(args, 0, sizeof(*args));
  args->max_search_subframes = 4000;

  while ((opt = getopt(argc, argv, "i:o:c:p:n:m:h")) != -1) {
    switch (opt) {
      case 'i':
        args->input_path = optarg;
        break;
      case 'o':
        args->output_path = optarg;
        break;
      case 'c':
        args->pci = (uint32_t)strtoul(optarg, NULL, 10);
        break;
      case 'p':
        args->nof_prb = (uint32_t)strtoul(optarg, NULL, 10);
        break;
      case 'n':
        args->nof_subframes = (uint32_t)strtoul(optarg, NULL, 10);
        break;
      case 'm':
        args->max_search_subframes = (uint32_t)strtoul(optarg, NULL, 10);
        break;
      case 'h':
      default:
        usage(argv[0]);
        return false;
    }
  }

  if (args->input_path == NULL || args->output_path == NULL || args->nof_prb == 0 || args->pci > 503) {
    usage(argv[0]);
    return false;
  }

  return true;
}

static int file_recv_callback(void* h, cf_t* x[SRSRAN_MAX_CHANNELS], uint32_t nsamples, srsran_timestamp_t* t)
{
  file_stream_t* stream = (file_stream_t*)h;

  (void)t;

  if (stream == NULL || stream->file == NULL || x == NULL || x[0] == NULL) {
    return SRSRAN_ERROR_INVALID_INPUTS;
  }

  size_t nread = fread(x[0], sizeof(cf_t), nsamples, stream->file);
  stream->samples_read += nread;

  if (nread != nsamples) {
    stream->eof = true;
    return SRSRAN_ERROR;
  }

  return (int)nread;
}

static bool write_subframe(FILE* out, const cf_t* buffer, uint32_t sf_len)
{
  return fwrite(buffer, sizeof(cf_t), sf_len, out) == sf_len;
}

int main(int argc, char** argv)
{
  args_t           args;
  FILE*            in                 = NULL;
  FILE*            out                = NULL;
  file_stream_t    stream             = {};
  srsran_ue_sync_t ue_sync;
  srsran_cell_t    cell;
  cf_t*            buffer0            = NULL;
  cf_t*            buffers[SRSRAN_MAX_CHANNELS] = {NULL};
  uint32_t         sf_len;
  uint32_t         max_num_samples;
  uint64_t         searched_subframes = 0;
  uint64_t         written_subframes  = 0;
  bool             started            = false;
  uint32_t         expected_sf_idx    = 0;
  int              ret                = EXIT_FAILURE;

  if (!parse_args(argc, argv, &args)) {
    return EXIT_FAILURE;
  }

  sf_len = SRSRAN_SF_LEN_PRB(args.nof_prb);
  if (sf_len == 0) {
    fprintf(stderr, "invalid PRB count: %u\n", args.nof_prb);
    return EXIT_FAILURE;
  }

  in = fopen(args.input_path, "rb");
  if (in == NULL) {
    fprintf(stderr, "failed to open input file %s: %s\n", args.input_path, strerror(errno));
    return EXIT_FAILURE;
  }

  out = fopen(args.output_path, "wb");
  if (out == NULL) {
    fprintf(stderr, "failed to open output file %s: %s\n", args.output_path, strerror(errno));
    fclose(in);
    return EXIT_FAILURE;
  }

  stream.file = in;

  max_num_samples = 3 * sf_len;
  buffer0         = srsran_vec_cf_malloc(max_num_samples);
  if (buffer0 == NULL) {
    fprintf(stderr, "failed to allocate sync buffer\n");
    goto cleanup;
  }
  buffers[0] = buffer0;

  memset(&ue_sync, 0, sizeof(ue_sync));
  if (srsran_ue_sync_init_multi(&ue_sync, args.nof_prb, false, file_recv_callback, 1, &stream) != SRSRAN_SUCCESS) {
    fprintf(stderr, "failed to initialize ue_sync\n");
    goto cleanup;
  }

  memset(&cell, 0, sizeof(cell));
  cell.id         = args.pci;
  cell.nof_prb    = args.nof_prb;
  cell.nof_ports  = 1;
  cell.cp         = SRSRAN_CP_NORM;
  cell.frame_type = SRSRAN_FDD;

  if (srsran_ue_sync_set_cell(&ue_sync, cell) != SRSRAN_SUCCESS) {
    fprintf(stderr, "failed to set LTE cell parameters in ue_sync\n");
    goto cleanup_sync;
  }

  ue_sync.cfo_current_value       = 0.0f;
  ue_sync.cfo_is_copied           = true;
  ue_sync.cfo_correct_enable_find = true;
  srsran_sync_set_cfo_cp_enable(&ue_sync.sfind, false, 0);

  fprintf(stdout,
          "syncing %s for PCI %u, PRB %u (sf_len=%u)\n",
          args.input_path,
          args.pci,
          args.nof_prb,
          sf_len);
  fflush(stdout);

  while (args.nof_subframes == 0 || written_subframes < args.nof_subframes) {
    int      sync_ret;
    uint32_t sf_idx;

    sync_ret = srsran_ue_sync_zerocopy(&ue_sync, buffers, max_num_samples);
    if (sync_ret < 0) {
      if (stream.eof) {
        break;
      }
      fprintf(stderr, "ue_sync failed while reading the raw file\n");
      goto cleanup_sync;
    }

    searched_subframes++;
    sf_idx = srsran_ue_sync_get_sfidx(&ue_sync);

    if (!started) {
      if ((searched_subframes % 100) == 0) {
        fprintf(stdout,
                "search progress: searched=%" PRIu64 " state=%d sf_idx=%u raw_samples=%" PRIu64 "\n",
                searched_subframes,
                ue_sync.state,
                sf_idx,
                stream.samples_read);
        fflush(stdout);
      }

      if (searched_subframes > args.max_search_subframes) {
        fprintf(stderr,
                "failed to lock to an LTE subframe-0 boundary within %u searched subframes\n",
                args.max_search_subframes);
        goto cleanup_sync;
      }

      if (sync_ret != 1 || sf_idx != 0) {
        continue;
      }

      started = true;
      fprintf(stdout,
              "locked: raw_samples=%" PRIu64 " current_sf_idx=%u cfo=%.1f Hz\n",
              stream.samples_read,
              sf_idx,
              15000.0f * ue_sync.cfo_current_value);
      fflush(stdout);
    }

    if (sync_ret != 1) {
      fprintf(stdout,
              "stopping after %" PRIu64 " written subframes due to lost sync at raw_samples=%" PRIu64 "\n",
              written_subframes,
              stream.samples_read);
      fflush(stdout);
      break;
    }

    if (sf_idx != expected_sf_idx) {
      fprintf(stdout,
              "stopping after %" PRIu64 " written subframes due to sf_idx jump (expected=%u got=%u)\n",
              written_subframes,
              expected_sf_idx,
              sf_idx);
      fflush(stdout);
      break;
    }

    if (!write_subframe(out, buffers[0], sf_len)) {
      fprintf(stderr, "failed to write synchronized subframe: %s\n", strerror(errno));
      goto cleanup_sync;
    }

    written_subframes++;
    expected_sf_idx = (expected_sf_idx + 1) % 10;
    if ((written_subframes % 1000) == 0) {
      fprintf(stdout, "written %" PRIu64 " synchronized subframes\n", written_subframes);
      fflush(stdout);
    }
  }

  if (!started || written_subframes == 0) {
    fprintf(stderr, "no synchronized subframes were written\n");
    goto cleanup_sync;
  }

  fprintf(stdout,
          "done: wrote %" PRIu64 " synchronized subframes to %s\n",
          written_subframes,
          args.output_path);
  ret = EXIT_SUCCESS;

cleanup_sync:
  srsran_ue_sync_free(&ue_sync);
cleanup:
  free(buffer0);
  if (out != NULL) {
    fclose(out);
  }
  if (in != NULL) {
    fclose(in);
  }

  return ret;
}
