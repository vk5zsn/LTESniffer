#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="${SCRIPT_DIR}/UhdCaptureRobust"

if [[ ! -x "${BIN}" ]]; then
  echo "missing ${BIN}; build the repo first" >&2
  exit 1
fi

if [[ $# -lt 1 ]]; then
  echo "usage: $0 OUTPUT_FILE [extra UhdCaptureRobust args...]" >&2
  exit 1
fi

output="$1"
shift

echo "writing to ${output}" >&2
if [[ "${output}" != /dev/shm/* ]]; then
  echo "warning: ${output} is not under /dev/shm; disk/USB contention may reduce capture robustness" >&2
fi

exec env LD_LIBRARY_PATH=/opt/install/uhd/lib \
  "${BIN}" \
  --output "${output}" \
  --args "type=b200,num_recv_frames=512" \
  --format sc16 \
  --wirefmt sc16 \
  --metadata \
  "$@"
