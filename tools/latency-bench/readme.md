# Latency Benchmarks (Karabiner + seq)

This toolkit gives maintainers a repeatable way to evaluate latency changes while making a PR merge-ready.

It benchmarks:
- Legacy shell-spawn path (`/bin/sh` + `seq`)
- `seq` CLI process path (`seq ping`, `seq run <macro>`, optional `open-app-toggle`)
- Direct socket path to `seqd` (`SOCK_STREAM` connect-per-call vs persistent, optional `SOCK_DGRAM`)

## Files
- `tools/latency-bench/bench_seq_latency.py`: run benchmark scenarios and emit percentiles.
- `tools/latency-bench/compare_runs.py`: compare two JSON runs (baseline vs candidate).
- `tools/latency-bench/run_pair_benchmark.sh`: run baseline/candidate refs and compare.
- `tools/latency-bench/run_regression_matrix.sh`: run targeted socket regressions + benchmark.

## Quick Start
Run from repo root:

```bash
./tools/latency-bench/bench_seq_latency.py \
  --iterations 300 \
  --warmup 50 \
  --macro-limit 8 \
  --json-out /tmp/kar_latency_baseline.json
```

The runner auto-extracts `seqSocket("...")` macros from `~/config/i/kar/config.ts` by default.

## Open-App Specific Benchmark
This includes the old shell fallback and `open-app-toggle` path:

```bash
./tools/latency-bench/bench_seq_latency.py \
  --iterations 150 \
  --warmup 30 \
  --include-open-app \
  --app Safari \
  --macro "open Safari new tab" \
  --json-out /tmp/kar_latency_open_app.json
```

## Compare Baseline vs Candidate (Manual)

```bash
./tools/latency-bench/compare_runs.py \
  --baseline /tmp/kar_latency_baseline.json \
  --candidate /tmp/kar_latency_candidate.json \
  --metric p95_us
```

## Compare Baseline vs Candidate (Automated)

```bash
./tools/latency-bench/run_pair_benchmark.sh \
  <baseline_ref> \
  <candidate_ref> \
  --iterations 300 --warmup 50 --macro-limit 8
```

This script checks out each ref, runs identical benchmark settings, compares p95, and writes artifacts under `/tmp/karabiner-latency-*`.

## Full Regression Matrix

```bash
./tools/latency-bench/run_regression_matrix.sh \
  --iterations 300 --warmup 50 --macro-limit 8
```

This runs:
1. `tests/src/socket_sender`
2. `tests/src/socket_command_handler`
3. `tests/src/post_event_to_virtual_devices`
4. Benchmark scenarios from `bench_seq_latency.py`

## Suggested Maintainer Protocol
1. Baseline on known-good commit.
2. Candidate on PR branch.
3. Reboot between runs if possible.
4. Keep same machine, same display refresh rate, same foreground apps.
5. Run each benchmark at least 3 times; compare median of p95 values.
6. Report both latency percentiles and failure counts.

## Interpreting Results
- `seq_cli_*` includes process spawn overhead.
- `socket_stream_connect_per_call` isolates connect/write/close overhead.
- `socket_stream_persistent` is closest to optimized Karabiner socket transport overhead.
- `socket_dgram_sendto` is best-case fire-and-forget transport (if seqd exposes `.dgram`).

## Notes
- Socket scenarios measure dispatch/send latency, not WindowServer compositing delay.
- For true keypress-to-photon latency, pair this with high-FPS camera tests.
- If `seqd` is not running, socket scenarios will fail (visible in `Fail` column).
