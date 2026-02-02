#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

echo "==> Targeted regression tests"
make -C tests/src/socket_sender
make -C tests/src/socket_command_handler
make -C tests/src/post_event_to_virtual_devices

echo
echo "==> Latency benchmark"
./tools/latency-bench/bench_seq_latency.py "$@"
