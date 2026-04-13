# Live Gold Capture Reference

Date: `2026-04-13`
Host: `vk2tlq@piSDR`
Repo: `/home/vk2tlq/LTESniffer`
Purpose: reference live decode run with simultaneous raw `cf32` capture for comparison against offline raw-IQ ingest.

## Command

```bash
sudo env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  /home/vk2tlq/LTESniffer/build/src/LTESniffer \
  -A 1 -W 4 -f 763e6 -C -m 0 -n 10000 \
  -a "num_recv_frames=512" \
  -Q /home/vk2tlq/live_gold_763M_11p52M_cf32.bin
```

## Key RF / Cell Results

- UHD: `4.9.0.0-0-g006d7f76`
- Device: `USRP B210`
- Clock: `23.04 MHz`
- Sample rate: `11.52 MHz`
- Frequency: `763.000 MHz`
- Cell search result: `PCI 405`
- Type: `FDD`
- CP: `Normal`
- PRB: `50`
- Nof ports: `2`
- PHICH Length: `Normal`
- PHICH Resources: `1`
- Decoded MIB SFN: `480`

## Processing Summary

- Requested subframes: `10000`
- Processed reports:
  - `[04:45:18] Processed 1000/1000 subframes`
  - `[04:45:19] Processed 1000/1000 subframes`
  - `[04:45:20] Processed 1000/1000 subframes`
  - `[04:45:21] Processed 1000/1000 subframes`
  - `[04:45:22] Processed 1000/1000 subframes`
  - `[04:45:23] Processed 1000/1000 subframes`
  - `[04:45:24] Processed 1000/1000 subframes`
  - `[04:45:25] Processed 1000/1000 subframes`
  - `[04:45:26] Processed 1000/1000 subframes`
  - `[04:45:27] Processed 1000/1000 subframes`
- Skipped subframes: `0 / 10000`
- Final stats:
  - `nof_decoded_locations=162350`
  - `nof_cce=221312`
  - `nof_missed_cce=2773`
  - `nof_subframes=9991`
  - `nof_subframe_collisions_dw=1`
  - `nof_subframe_collisions_up=4`
  - `nof_locations=408723`

## RNTI Summary

- Final 64QAM-table RNTIs: `13`
- Final 256QAM-table RNTIs: `0`
- Final Unknown-table RNTIs: `5`

64QAM table snapshot:

- `10690`: active `120`, success `102 (85%)`
- `11035`: active `7`, success `4 (57%)`
- `11122`: active `6`, success `6 (100%)`
- `11131`: active `66`, success `38 (58%)`
- `11384`: active `17`, success `5 (29%)`
- `11479`: active `2`, success `2 (100%)`
- `11553`: active `17`, success `15 (88%)`
- `11555`: active `285`, success `180 (63%)`
- `11598`: active `19`, success `5 (26%)`
- `11697`: active `35`, success `28 (80%)`
- `11754`: active `12`, success `12 (100%)`
- `11856`: active `122`, success `44 (36%)`
- `11971`: active `21`, success `15 (71%)`

Unknown table snapshot:

- `10761`: active `28`, success `0`
- `10828`: active `7`, success `0`
- `11166`: active `1507`, success `7`
- `11635`: active `2`, success `1`
- `11864`: active `1741`, success `2`

## Raw IQ Capture Summary

- Output file: `/home/vk2tlq/live_gold_763M_11p52M_cf32.bin`
- Live raw IQ capture summary:
  - `overflows=0`
  - `late_rx=0`
  - `rx_errors=0`
  - `other_errors=0`
  - `timestamp_gap_events=0`
  - `timestamp_gap_samples_accum=0`
  - `timestamp_gap_samples_max_abs=0`
  - `file_write_failed=0`

## Interpretation

This run is the current gold-standard reference showing:

- live decode works well on `PCI 405 / PRB 50`
- simultaneous raw `cf32` recording via `-Q` completed without reported overflows or timestamp gaps
- if offline raw ingest behaves worse on the resulting file, the cause is less likely to be UHD capture overruns and more likely to be in offline synchronization / replay handling
