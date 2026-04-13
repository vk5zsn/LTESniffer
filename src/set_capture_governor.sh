#!/usr/bin/env bash
set -euo pipefail

mode="${1:-performance}"

for f in /sys/devices/system/cpu/cpufreq/policy*/scaling_governor; do
  printf '%s\n' "$mode" | sudo tee "$f" >/dev/null
done

for f in /sys/devices/system/cpu/cpufreq/policy*/scaling_governor; do
  printf '%s ' "$f"
  cat "$f"
done
