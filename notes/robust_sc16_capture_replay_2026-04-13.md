# Robust sc16 Capture and Replay Baseline (2026-04-13)

## Setup

- Host: Raspberry Pi 5
- B210: externally powered
- USB topology: B210 moved to a different USB 3 root branch from the SSD
- CPU governor: `performance`
- UHD runtime: `/opt/install/uhd/lib`, version `4.9.0.0-0-g006d7f76`
- Capture format: raw interleaved `sc16`
- Replay mode: direct raw ingest with `-j -J`

## USB topology check

```text
B210: Bus 04, directly under 5000M root hub
SSD:  Bus 02, under separate 5000M root hub and hub chain
```

This confirms the B210 is no longer sharing the SSD's USB branch.

## Capture command

```bash
sudo env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  /home/vk2tlq/LTESniffer/build/src/UhdCaptureRobust \
  --output /dev/shm/iq_763M_11p52M_sc16.bin \
  --args "type=b200,num_recv_frames=512,master_clock_rate=23.04e6" \
  --freq 763e6 \
  --rate 11.52e6 \
  --gain 50 \
  --antenna RX2 \
  --duration 10 \
  --format sc16 \
  --wirefmt sc16 \
  --metadata
```

## Capture result

```text
Captured 114626560 samples to /dev/shm/iq_763M_11p52M_sc16.bin
Format sc16, elapsed 10.001 s, write throughput 45.845 MB/s
overflows=0 timeouts=0 late=0 other_errors=0
```

Derived values:

- Bytes written: `458506240`
- Approximate LTE subframes in file: `114626560 / 11520 = 9950.22`

## Replay command

```bash
env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  /home/vk2tlq/LTESniffer/build/src/LTESniffer \
  -A 1 -W 4 \
  -i /dev/shm/iq_763M_11p52M_sc16.bin \
  -j -J \
  -P 2 -c 405 -p 50 -G 1.0 -m 0 -n 9950 -d
```

## Replay result

Key observations:

- Locked immediately:
  - `Raw input stream locked at raw_samples=81783`
- Valid MIB:
  - PCI: `405`
  - PRB: `50`
  - Ports: `2`
  - PHICH Resources: `1`
  - SFN: `248`
- File ran to EOF with the expected final read error from `ue_sync`

Final counters:

```text
Skipped subframe: 0 / 9944
nof_decoded_locations, nof_cce, nof_missed_cce, nof_subframes, nof_subframe_collisions_dw, nof_subframe_collisions_up, time, nof_locations
63229, 165327, 308, 9865, 1, 0, 0.000000, 305978
Skipped subframes: 0 (0%)
```

RNTI summary:

```text
10761  active=13  success=3
11005  active=3   success=3
11077  active=55  success=26
11147  active=14  success=4
11151  active=7   success=1
11210  active=21  success=20
11281  active=15  success=4
11555  active=102 success=87
11766  active=165 success=163
11864  active=48  success=3
```

## Interpretation

- The robust `sc16` capture path is stable on this Pi5 and B210 setup.
- The capture itself shows zero transport errors at 11.52 Msps for 10 seconds.
- Direct replay through `LTESniffer -j -J` works across essentially the full file and produces substantial decode activity.
- This is now the baseline raw-IQ workflow to compare against future captures from other tools.
