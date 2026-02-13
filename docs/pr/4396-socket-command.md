# PR #4396 Benchmark Guide: socket_command Latency

This document describes a reproducible benchmark protocol for evaluating PR #4396
("socket_command" support) against baseline behavior.

Scope:
- Measure transport and process-overhead latency (`shell` vs `seq` CLI vs socket).
- Validate functional regressions in socket-specific code paths.
- Produce PR-ready artifacts (JSON outputs + p95 deltas).

Out of scope:
- True keypress-to-photon latency (requires camera test; optional section below).

## 1. Test Goal

We want to verify that the new socket path is:
1. Correct (no regressions in socket sender/handler/manipulator behavior).
2. Faster than shell/CLI dispatch paths.
3. Stable across repeated runs on a separate Mac.

Primary KPI:
- `p95_us` and `p99_us` for each scenario.

Secondary KPI:
- `failed` count should be `0` for healthy runs.

## 2. Environment Requirements

Run on a separate Mac (recommended), with:
- same keyboard/display refresh profile you care about,
- no heavy foreground workloads,
- `seqd` running and reachable at `/tmp/seqd.sock`.

Required files in this repo:
- `tools/latency-bench/bench_seq_latency.py`
- `tools/latency-bench/compare_runs.py`
- `tools/latency-bench/run_pair_benchmark.sh`
- `tools/latency-bench/run_regression_matrix.sh`

Required external binary:
- `~/code/seq/cli/cpp/out/bin/seq`

Quick check:

```bash
~/code/seq/cli/cpp/out/bin/seq ping
ls -l /tmp/seqd.sock
```

## 3. Benchmark Scenarios

The benchmark runner executes:

- `seq_cli_ping`
  - Cost of spawning `seq` process and doing minimal IPC.
- `seq_cli_run:<macro>`
  - Cost of spawning `seq` process for real macro calls (from `seqSocket("...")` in config).
- `legacy_shell_open_app_toggle` (optional)
  - Old shell-style dispatch (`/bin/sh` + `seq open-app-toggle` fallback chain).
- `seq_cli_open_app_toggle` (optional)
  - Non-shell process path for app activation.
- `socket_stream_connect_per_call`
  - Connect + send + close on each command.
- `socket_stream_persistent`
  - Reused stream socket connection (target path for this PR).
- `socket_dgram_sendto` (if available)
  - Datagram fire-and-forget (best-case transport floor).

## 4. Baseline vs Candidate Procedure

Use fixed refs:
- `BASELINE_REF`: known good commit before PR behavior.
- `CANDIDATE_REF`: PR head (`socket-command` / corresponding SHA).

### 4.1 Automated pair run

```bash
./tools/latency-bench/run_pair_benchmark.sh \
  <baseline_ref> \
  <candidate_ref> \
  --iterations 300 \
  --warmup 50 \
  --macro-limit 8
```

What it does:
1. Checks out baseline ref and runs benchmark.
2. Checks out candidate ref and runs benchmark.
3. Compares `p95_us` automatically.
4. Stores artifacts in `/tmp/karabiner-latency-<timestamp>/`.

### 4.2 Manual pair run (if you want custom control)

```bash
# Baseline
./tools/latency-bench/bench_seq_latency.py \
  --iterations 300 \
  --warmup 50 \
  --macro-limit 8 \
  --json-out /tmp/kar_latency_baseline.json

# Candidate
./tools/latency-bench/bench_seq_latency.py \
  --iterations 300 \
  --warmup 50 \
  --macro-limit 8 \
  --json-out /tmp/kar_latency_candidate.json

# Compare
./tools/latency-bench/compare_runs.py \
  --baseline /tmp/kar_latency_baseline.json \
  --candidate /tmp/kar_latency_candidate.json \
  --metric p95_us
```

## 5. Open-App Focused Test

To specifically evaluate `open-app-toggle` behavior:

```bash
./tools/latency-bench/bench_seq_latency.py \
  --iterations 150 \
  --warmup 30 \
  --include-open-app \
  --app Safari \
  --macro "open Safari new tab" \
  --json-out /tmp/kar_latency_open_app.json
```

This captures differences between:
- legacy shell dispatch,
- direct `seq` CLI open-app dispatch,
- direct socket dispatch transport overhead.

## 6. Regression Matrix (Correctness + Perf)

Run targeted tests before posting benchmark numbers:

```bash
./tools/latency-bench/run_regression_matrix.sh \
  --iterations 300 \
  --warmup 50 \
  --macro-limit 8
```

This runs:
1. `tests/src/socket_sender`
2. `tests/src/socket_command_handler`
3. `tests/src/post_event_to_virtual_devices`
4. latency benchmark scenarios

If any test fails, do not compare latency numbers yet.

## 7. Noise Control Rules

For stable comparisons:
- Use the same machine for baseline and candidate.
- Keep same display refresh rate and power state.
- Close browser tabs/IDE indexing/background heavy tasks.
- Run each pair at least 3 times.
- Report median of run-level `p95_us`.

## 8. Reporting Template for PR

Use this exact structure in PR comments:

```text
Machine: <model>, macOS <version>
Seq: <seq commit or build id>
Karabiner ref baseline: <sha>
Karabiner ref candidate: <sha>
Iterations/warmup: 300/50

Result (metric: p95_us):
- seq_cli_ping: <baseline> -> <candidate> (<delta %>)
- seq_cli_run:... : <baseline> -> <candidate> (<delta %>)
- socket_stream_connect_per_call: <baseline> -> <candidate> (<delta %>)
- socket_stream_persistent: <baseline> -> <candidate> (<delta %>)
- socket_dgram_sendto: <baseline> -> <candidate> (<delta %>)

Failures:
- baseline failed count: <n>
- candidate failed count: <n>

Artifacts:
- /tmp/karabiner-latency-<timestamp>/baseline.json
- /tmp/karabiner-latency-<timestamp>/candidate.json
```

## 9. Optional: Keypress-to-Photon Validation

Use only if you need true user-perceived latency numbers.

Method:
1. Keep one key bound to legacy path and one to socket path in your Karabiner config.
2. Use 240fps+ camera to record keypress and visible app/window change.
3. Measure frame deltas across at least 30 trials per path.
4. Report median and p95 in milliseconds.

This is separate from transport microbenchmarks and includes display compositor + panel latency.

## 10. Expected Direction (Not Guaranteed Absolute)

Under normal conditions, candidate should improve:
- `socket_stream_persistent` vs process-spawn paths by a large margin.
- `socket_stream_connect_per_call` vs shell/CLI by a significant margin.

Absolute values vary by machine load and seqd state. Compare deltas, not single-run absolutes.
