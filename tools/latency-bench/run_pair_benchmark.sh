#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  cat <<'USAGE'
Usage: tools/latency-bench/run_pair_benchmark.sh <baseline_ref> <candidate_ref> [bench args...]

Example:
  tools/latency-bench/run_pair_benchmark.sh \
    a1b2c3d \
    feature/socket-fastpath \
    --iterations 300 --warmup 50 --macro-limit 8
USAGE
  exit 1
fi

BASELINE_REF="$1"
CANDIDATE_REF="$2"
shift 2

if [[ -n "$(git status --porcelain)" ]]; then
  echo "Refusing to run: working tree is dirty." >&2
  echo "Commit or stash changes first, then re-run." >&2
  exit 2
fi

ORIG_REF="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$ORIG_REF" == "HEAD" ]]; then
  ORIG_REF="$(git rev-parse HEAD)"
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT_DIR="/tmp/karabiner-latency-${STAMP}"
mkdir -p "$OUT_DIR"

restore_ref() {
  git checkout -q "$ORIG_REF" || true
}
trap restore_ref EXIT

run_one() {
  local ref="$1"
  local out_json="$2"
  shift 2

  echo
  echo "==> Benchmarking ${ref}"
  git checkout -q "$ref"
  ./tools/latency-bench/bench_seq_latency.py "$@" --json-out "$out_json"
}

run_one "$BASELINE_REF" "$OUT_DIR/baseline.json" "$@"
run_one "$CANDIDATE_REF" "$OUT_DIR/candidate.json" "$@"

echo
echo "==> Comparison (p95)"
./tools/latency-bench/compare_runs.py \
  --baseline "$OUT_DIR/baseline.json" \
  --candidate "$OUT_DIR/candidate.json" \
  --metric p95_us

echo
echo "Artifacts: $OUT_DIR"
