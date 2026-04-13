# Gqrx Raw Replay Compare (2026-04-13)

## Input file

- Path: `/home/vk2tlq/gqrx_20260413_050750_763000000_11520000_fc.raw`
- Size: `322642280` bytes
- Approximate LTE subframes by size at 11.52 Msps cf32: `3500.8928`
- Filename center frequency: `763000000`

## Command

```bash
env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  /home/vk2tlq/LTESniffer/build/src/LTESniffer \
  -A 1 -W 4 \
  -i /home/vk2tlq/gqrx_20260413_050750_763000000_11520000_fc.raw \
  -j \
  -P 2 -c 405 -p 50 -G 1.0 -m 0 -n 3500 -d
```

## Key observations

- Direct raw-stream ingest (`-j`) locked immediately without any `-o` frequency correction.
- Valid MIB decoded:
  - PCI: `405`
  - PRB: `50`
  - Ports: `2`
  - PHICH Resources: `1`
  - SFN: `676`
- File ended normally with the expected `Error receiving samples` message at EOF.

## Final counters

```text
Skipped subframe: 0 / 3490
nof_decoded_locations, nof_cce, nof_missed_cce, nof_subframes, nof_subframe_collisions_dw, nof_subframe_collisions_up, time, nof_locations
107557, 109016, 1029, 3462, 2, 12, 0.000000, 201556
Skipped subframes: 0 (0%)
```

## RNTI summary

```text
64QAM:
10761  active=3    success=1
11054  active=98   success=94
11291  active=3    success=3
11431  active=50   success=49
11555  active=117  success=96
11580  active=630  success=594
11622  active=1158 success=73
11864  active=794  success=2

Unknown:
10784  active=2 success=2
11111  active=1 success=1
11456  active=4 success=0
```

## Interpretation

- This new Gqrx capture behaves like the gold `-Q` capture in the important sense: raw file replay through `-j` works without extra frequency correction.
- The earlier failing Gqrx file was therefore consistent with the identified `20 kHz` Gqrx tuning/correction mistake, not a generic raw-IQ ingest limitation.
