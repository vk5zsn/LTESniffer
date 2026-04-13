# Live Gold Replay Comparison

Date: `2026-04-13`
Host: `vk2tlq@piSDR`
Repo: `/home/vk2tlq/LTESniffer`
Branch: `LTESniffer-raw-file-sync-input`
Reference live capture note: [live_gold_capture_2026-04-13.md](/home/vk2tlq/LTESniffer/notes/live_gold_capture_2026-04-13.md)

Purpose: compare the archived live `-Q` gold `cf32` capture against the direct raw-stream offline ingest path (`-i ... -j`).

## Replay Command

```bash
env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  /home/vk2tlq/LTESniffer/build/src/LTESniffer \
  -A 1 -W 4 \
  -i /home/vk2tlq/live_gold_763M_11p52M_cf32.bin \
  -j \
  -P 2 -c 405 -p 50 -G 1.0 -m 0 -n 10000 -d
```

## Gold File

- Input file: `/home/vk2tlq/live_gold_763M_11p52M_cf32.bin`
- File size: `922148560` bytes
- Size-derived LTE subframes at `11.52 Msps`: `10005.952256944445`

## Offline Replay Results

- Direct raw stream lock: `raw_samples=172040`
- MIB decode:
  - `PCI 405`
  - `PRB 50`
  - `ports 2`
  - `PHICH Resources 1`
  - `SFN 484`
- Final stats:
  - `nof_decoded_locations=159681`
  - `nof_cce=219694`
  - `nof_missed_cce=2780`
  - `nof_subframes=9938`
  - `nof_subframe_collisions_dw=2`
  - `nof_subframe_collisions_up=7`
  - `nof_locations=405726`
- Skipped subframes: `0 / 9992`

Final RNTI summary:

- `64QAM`: `15`
- `256QAM`: `0`
- `Unknown`: `3`

64QAM table snapshot:

- `10690`: active `112`, success `94 (84%)`
- `11035`: active `5`, success `4 (80%)`
- `11122`: active `6`, success `6 (100%)`
- `11131`: active `52`, success `30 (58%)`
- `11166`: active `1499`, success `8 (1%)`
- `11384`: active `19`, success `6 (32%)`
- `11479`: active `2`, success `2 (100%)`
- `11553`: active `15`, success `14 (93%)`
- `11555`: active `274`, success `176 (64%)`
- `11598`: active `21`, success `7 (33%)`
- `11697`: active `35`, success `29 (83%)`
- `11754`: active `13`, success `13 (100%)`
- `11856`: active `121`, success `43 (36%)`
- `11864`: active `1660`, success `2 (0%)`
- `11971`: active `22`, success `16 (73%)`

Unknown table snapshot:

- `10761`: active `28`, success `0`
- `10828`: active `6`, success `0`
- `11635`: active `2`, success `1`

Note:

- End of run printed:
  - `Finish reading raw input file`
  - `ue_sync.c:772: Error receiving samples`
- This is expected at EOF for the raw file path and does not invalidate the summary counters above.

## Live vs Replay

Reference live run from [live_gold_capture_2026-04-13.md](/home/vk2tlq/LTESniffer/notes/live_gold_capture_2026-04-13.md):

- live `nof_decoded_locations=162350`
- replay `nof_decoded_locations=159681`
- delta: `-2669` (`-1.64%`)

- live `nof_cce=221312`
- replay `nof_cce=219694`
- delta: `-1618` (`-0.73%`)

- live `nof_subframes=9991`
- replay `nof_subframes=9938`
- delta: `-53` (`-0.53%`)

- live `nof_locations=408723`
- replay `nof_locations=405726`
- delta: `-2997` (`-0.73%`)

## Interpretation

This comparison shows that the direct raw-stream `-j` path can reproduce live decode behavior closely when the input file is the live `-Q` capture generated from the same `LTESniffer` receive path.

That strongly suggests the earlier poor behavior on the externally captured UHD raw file is not simply because “raw `cf32` from disk cannot work.” The remaining difference is more likely in the external capture path itself, such as:

- dropped or discontinuous samples before replay
- different buffering / streaming behavior outside `LTESniffer`
- capture alignment / integrity differences in the independently recorded file
