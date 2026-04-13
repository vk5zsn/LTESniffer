#include <uhd/exception.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread_priority.hpp>

#include <boost/program_options.hpp>

#include <atomic>
#include <chrono>
#include <complex>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = boost::program_options;

namespace {

std::atomic<bool> stop_signal_called(false);

void sig_int_handler(int)
{
  stop_signal_called = true;
}

struct CaptureStats {
  uint64_t total_samples = 0;
  uint64_t overflow_count = 0;
  uint64_t timeout_count = 0;
  uint64_t late_count = 0;
  uint64_t other_error_count = 0;
};

template <typename SampleT>
CaptureStats capture_to_file(uhd::usrp::multi_usrp::sptr usrp,
                             const std::string& cpu_format,
                             const std::string& wire_format,
                             const std::string& stream_args_extra,
                             const std::string& output_path,
                             uint64_t nsamps,
                             double duration_s)
{
  uhd::stream_args_t stream_args(cpu_format, wire_format);
  stream_args.channels = {0};
  if (!stream_args_extra.empty()) {
    stream_args.args = stream_args_extra;
  }

  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
  const size_t samps_per_buff = std::max<size_t>(rx_stream->get_max_num_samps(), 4096);
  std::vector<SampleT> buffer(samps_per_buff);
  std::ofstream output(output_path.c_str(), std::ios::binary);
  if (!output.is_open()) {
    throw std::runtime_error("failed to open output file");
  }

  uhd::stream_cmd_t stream_cmd(nsamps > 0 ? uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
                                          : uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  stream_cmd.stream_now = false;
  stream_cmd.num_samps  = nsamps;
  stream_cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
  rx_stream->issue_stream_cmd(stream_cmd);

  CaptureStats stats;
  uhd::rx_metadata_t md;
  const auto start = std::chrono::steady_clock::now();
  const auto stop_time = duration_s > 0.0 ? start + std::chrono::duration<double>(duration_s)
                                          : std::chrono::steady_clock::time_point::max();

  while (!stop_signal_called.load()) {
    if (duration_s > 0.0 && std::chrono::steady_clock::now() >= stop_time) {
      break;
    }
    if (nsamps > 0 && stats.total_samples >= nsamps) {
      break;
    }

    size_t request = samps_per_buff;
    if (nsamps > 0) {
      request = std::min<uint64_t>(request, nsamps - stats.total_samples);
    }

    const size_t num_rx = rx_stream->recv(buffer.data(), request, md, 1.0, false);

    if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
      stats.timeout_count++;
      continue;
    }
    if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
      stats.overflow_count++;
      if (num_rx == 0) {
        continue;
      }
    } else if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND) {
      stats.late_count++;
      continue;
    } else if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
      stats.other_error_count++;
      if (num_rx == 0) {
        continue;
      }
    }

    output.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(num_rx * sizeof(SampleT)));
    if (!output.good()) {
      throw std::runtime_error("failed while writing capture file");
    }
    stats.total_samples += num_rx;
  }

  if (nsamps == 0) {
    uhd::stream_cmd_t stop_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    rx_stream->issue_stream_cmd(stop_cmd);
  }

  output.flush();
  return stats;
}

void write_metadata_file(const std::string& output_path,
                         const po::variables_map& vm,
                         const CaptureStats& stats,
                         double elapsed_s)
{
  std::ofstream meta((output_path + ".meta.txt").c_str(), std::ios::out);
  if (!meta.is_open()) {
    throw std::runtime_error("failed to open metadata sidecar");
  }

  meta << std::fixed << std::setprecision(6);
  meta << "output=" << output_path << "\n";
  meta << "device_args=" << vm["args"].as<std::string>() << "\n";
  meta << "stream_args=" << vm["stream-args"].as<std::string>() << "\n";
  meta << "freq_hz=" << vm["freq"].as<double>() << "\n";
  meta << "rate_sps=" << vm["rate"].as<double>() << "\n";
  meta << "gain_db=" << vm["gain"].as<double>() << "\n";
  meta << "antenna=" << vm["antenna"].as<std::string>() << "\n";
  meta << "format=" << vm["format"].as<std::string>() << "\n";
  meta << "wirefmt=" << vm["wirefmt"].as<std::string>() << "\n";
  meta << "nsamps_requested=" << vm["nsamps"].as<uint64_t>() << "\n";
  meta << "duration_requested_s=" << vm["duration"].as<double>() << "\n";
  meta << "elapsed_s=" << elapsed_s << "\n";
  meta << "samples_captured=" << stats.total_samples << "\n";
  meta << "overflows=" << stats.overflow_count << "\n";
  meta << "timeouts=" << stats.timeout_count << "\n";
  meta << "late=" << stats.late_count << "\n";
  meta << "other_errors=" << stats.other_error_count << "\n";
}

} // namespace

int UHD_SAFE_MAIN(int argc, char** argv)
{
  std::signal(SIGINT, &sig_int_handler);
  uhd::set_thread_priority_safe();

  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "show help")
    ("output,o", po::value<std::string>()->required(), "output file")
    ("args,a", po::value<std::string>()->default_value("type=b200,num_recv_frames=512"), "UHD device args")
    ("stream-args", po::value<std::string>()->default_value(""), "UHD stream args")
    ("spec", po::value<std::string>()->default_value(""), "subdevice spec")
    ("freq,f", po::value<double>()->required(), "center frequency in Hz")
    ("rate,r", po::value<double>()->required(), "sample rate in S/s")
    ("gain,g", po::value<double>()->default_value(50.0), "RX gain in dB")
    ("antenna,A", po::value<std::string>()->default_value("RX2"), "RX antenna")
    ("duration,d", po::value<double>()->default_value(0.0), "capture duration in seconds")
    ("nsamps,N", po::value<uint64_t>()->default_value(0), "number of samples to capture")
    ("format,F", po::value<std::string>()->default_value("sc16"), "file sample format: sc16 or fc32")
    ("wirefmt,W", po::value<std::string>()->default_value("sc16"), "wire format")
    ("metadata,m", po::bool_switch()->default_value(false), "write .meta.txt sidecar");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }
    po::notify(vm);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << desc << std::endl;
    return 1;
  }

  const std::string format = vm["format"].as<std::string>();
  if (format != "sc16" && format != "fc32") {
    std::cerr << "Unsupported format: " << format << std::endl;
    return 1;
  }

  if (vm["duration"].as<double>() <= 0.0 && vm["nsamps"].as<uint64_t>() == 0) {
    std::cerr << "Either --duration or --nsamps must be specified" << std::endl;
    return 1;
  }

  auto usrp = uhd::usrp::multi_usrp::make(vm["args"].as<std::string>());
  if (!vm["spec"].as<std::string>().empty()) {
    usrp->set_rx_subdev_spec(vm["spec"].as<std::string>());
  }

  usrp->set_rx_rate(vm["rate"].as<double>());
  usrp->set_rx_gain(vm["gain"].as<double>(), 0);
  usrp->set_rx_antenna(vm["antenna"].as<std::string>(), 0);
  usrp->set_rx_freq(uhd::tune_request_t(vm["freq"].as<double>()), 0);

  const auto start = std::chrono::steady_clock::now();
  CaptureStats stats;
  if (format == "sc16") {
    stats = capture_to_file<std::complex<int16_t>>(usrp,
                                                   "sc16",
                                                   vm["wirefmt"].as<std::string>(),
                                                   vm["stream-args"].as<std::string>(),
                                                   vm["output"].as<std::string>(),
                                                   vm["nsamps"].as<uint64_t>(),
                                                   vm["duration"].as<double>());
  } else {
    stats = capture_to_file<std::complex<float>>(usrp,
                                                 "fc32",
                                                 vm["wirefmt"].as<std::string>(),
                                                 vm["stream-args"].as<std::string>(),
                                                 vm["output"].as<std::string>(),
                                                 vm["nsamps"].as<uint64_t>(),
                                                 vm["duration"].as<double>());
  }
  const double elapsed_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  const double bytes_per_sample = format == "sc16" ? sizeof(std::complex<int16_t>) : sizeof(std::complex<float>);
  const double throughput_mb_s = elapsed_s > 0.0 ? (stats.total_samples * bytes_per_sample) / elapsed_s / 1e6 : 0.0;

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "Captured " << stats.total_samples << " samples to " << vm["output"].as<std::string>() << std::endl;
  std::cout << "Format " << format << ", elapsed " << elapsed_s << " s, write throughput "
            << throughput_mb_s << " MB/s" << std::endl;
  std::cout << "overflows=" << stats.overflow_count
            << " timeouts=" << stats.timeout_count
            << " late=" << stats.late_count
            << " other_errors=" << stats.other_error_count << std::endl;

  if (vm["metadata"].as<bool>()) {
    write_metadata_file(vm["output"].as<std::string>(), vm, stats, elapsed_s);
  }

  return 0;
}
