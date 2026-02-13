#!/usr/bin/env python3

import argparse
import dataclasses
import json
import math
import os
import platform
import re
import socket
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import Callable, Optional


@dataclasses.dataclass
class ScenarioResult:
    name: str
    ok: int
    failed: int
    samples_us: list[float]
    notes: str = ""

    def summary(self) -> dict[str, object]:
        data = sorted(self.samples_us)
        return {
            "name": self.name,
            "ok": self.ok,
            "failed": self.failed,
            "notes": self.notes,
            "min_us": _round(_safe_min(data)),
            "p50_us": _round(_percentile(data, 50)),
            "p90_us": _round(_percentile(data, 90)),
            "p95_us": _round(_percentile(data, 95)),
            "p99_us": _round(_percentile(data, 99)),
            "mean_us": _round(_safe_mean(data)),
            "stdev_us": _round(_safe_stdev(data)),
            "max_us": _round(_safe_max(data)),
        }


def _round(value: Optional[float]) -> Optional[float]:
    if value is None:
        return None
    return round(value, 3)


def _safe_min(xs: list[float]) -> Optional[float]:
    return min(xs) if xs else None


def _safe_max(xs: list[float]) -> Optional[float]:
    return max(xs) if xs else None


def _safe_mean(xs: list[float]) -> Optional[float]:
    return statistics.fmean(xs) if xs else None


def _safe_stdev(xs: list[float]) -> Optional[float]:
    return statistics.pstdev(xs) if len(xs) > 1 else 0.0 if xs else None


def _percentile(xs: list[float], pct: float) -> Optional[float]:
    if not xs:
        return None
    if len(xs) == 1:
        return xs[0]
    pos = (len(xs) - 1) * (pct / 100.0)
    lo = math.floor(pos)
    hi = math.ceil(pos)
    if lo == hi:
        return xs[lo]
    lower = xs[lo]
    upper = xs[hi]
    return lower + (upper - lower) * (pos - lo)


def run_scenario(
    name: str,
    iterations: int,
    warmup: int,
    sleep_ms: float,
    probe: Callable[[], bool],
    notes: str = "",
) -> ScenarioResult:
    for _ in range(warmup):
        try:
            probe()
        except Exception:
            pass

    samples_us: list[float] = []
    ok = 0
    failed = 0

    for _ in range(iterations):
        t0 = time.perf_counter_ns()
        success = False
        try:
            success = probe()
        except Exception:
            success = False
        dt_us = (time.perf_counter_ns() - t0) / 1_000.0

        if success:
            ok += 1
            samples_us.append(dt_us)
        else:
            failed += 1

        if sleep_ms > 0:
            time.sleep(sleep_ms / 1000.0)

    return ScenarioResult(name=name, ok=ok, failed=failed, samples_us=samples_us, notes=notes)


def make_subprocess_probe(cmd: list[str], timeout_s: float) -> Callable[[], bool]:
    def probe() -> bool:
        p = subprocess.run(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=timeout_s,
            check=False,
        )
        return p.returncode == 0

    return probe


def make_shell_probe(shell_command: str, timeout_s: float) -> Callable[[], bool]:
    return make_subprocess_probe(["/bin/sh", "-lc", shell_command], timeout_s)


def make_stream_probe_connect_per_call(
    endpoint: str,
    payload: bytes,
    timeout_s: float,
) -> Callable[[], bool]:
    def probe() -> bool:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(timeout_s)
        try:
            s.connect(endpoint)
            s.sendall(payload)
            return True
        finally:
            s.close()

    return probe


class PersistentStreamProbe:
    def __init__(self, endpoint: str, timeout_s: float):
        self.endpoint = endpoint
        self.timeout_s = timeout_s
        self.sock: Optional[socket.socket] = None

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.close()
            finally:
                self.sock = None

    def _connect(self) -> None:
        self.close()
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(self.timeout_s)
        sock.connect(self.endpoint)
        self.sock = sock

    def send(self, payload: bytes) -> bool:
        for _ in range(2):
            try:
                if self.sock is None:
                    self._connect()
                assert self.sock is not None
                self.sock.sendall(payload)
                return True
            except OSError:
                self.close()
        return False


def make_stream_probe_persistent(
    endpoint: str,
    payload: bytes,
    timeout_s: float,
) -> tuple[Callable[[], bool], PersistentStreamProbe]:
    sender = PersistentStreamProbe(endpoint=endpoint, timeout_s=timeout_s)

    def probe() -> bool:
        return sender.send(payload)

    return probe, sender


def make_dgram_probe(endpoint: str, payload: bytes, timeout_s: float) -> tuple[Callable[[], bool], socket.socket]:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.settimeout(timeout_s)
    target = endpoint + ".dgram"

    def probe() -> bool:
        sent = sock.sendto(payload, target)
        return sent == len(payload)

    return probe, sock


def extract_seq_socket_macros(config_path: Path, limit: int) -> list[str]:
    if not config_path.exists():
        return []

    text = config_path.read_text(encoding="utf-8", errors="ignore")
    pattern = re.compile(r'seqSocket\(\s*"([^"]+)"')
    matches = pattern.findall(text)

    deduped: list[str] = []
    seen: set[str] = set()
    for m in matches:
        if m in seen:
            continue
        seen.add(m)
        deduped.append(m)
        if len(deduped) >= limit:
            break
    return deduped


def print_summary(results: list[ScenarioResult]) -> None:
    headers = [
        "Scenario",
        "OK",
        "Fail",
        "p50(us)",
        "p95(us)",
        "p99(us)",
        "mean(us)",
        "min(us)",
        "max(us)",
    ]
    rows: list[list[str]] = [headers]

    for r in results:
        s = r.summary()
        rows.append(
            [
                r.name,
                str(s["ok"]),
                str(s["failed"]),
                _fmt(s["p50_us"]),
                _fmt(s["p95_us"]),
                _fmt(s["p99_us"]),
                _fmt(s["mean_us"]),
                _fmt(s["min_us"]),
                _fmt(s["max_us"]),
            ]
        )

    widths = [max(len(row[i]) for row in rows) for i in range(len(headers))]
    for idx, row in enumerate(rows):
        line = "  ".join(value.ljust(widths[i]) for i, value in enumerate(row))
        print(line)
        if idx == 0:
            print("  ".join("-" * width for width in widths))


def _fmt(v: object) -> str:
    if v is None:
        return "-"
    if isinstance(v, float):
        return f"{v:.3f}"
    return str(v)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Benchmark seq/karabiner latency paths (shell, seq CLI, unix socket)."
    )
    parser.add_argument("--iterations", type=int, default=200)
    parser.add_argument("--warmup", type=int, default=30)
    parser.add_argument("--sleep-ms", type=float, default=0.0)
    parser.add_argument("--timeout-ms", type=int, default=3000)
    parser.add_argument("--seq-bin", default="~/code/seq/cli/cpp/out/bin/seq")
    parser.add_argument("--seq-socket", default="/tmp/seqd.sock")
    parser.add_argument("--config", default="~/config/i/kar/config.ts")
    parser.add_argument("--macro", action="append", default=[])
    parser.add_argument("--macro-limit", type=int, default=6)
    parser.add_argument("--include-open-app", action="store_true")
    parser.add_argument("--app", default="Safari")
    parser.add_argument("--json-out", default="")
    args = parser.parse_args()

    timeout_s = args.timeout_ms / 1000.0
    seq_bin = os.path.expanduser(args.seq_bin)
    seq_socket = os.path.expanduser(args.seq_socket)
    config_path = Path(os.path.expanduser(args.config))

    explicit_macros = [m for m in args.macro if m.strip()]
    auto_macros = extract_seq_socket_macros(config_path, max(args.macro_limit, 0))
    macros = explicit_macros + [m for m in auto_macros if m not in explicit_macros]

    print(f"Host: {platform.node()} ({platform.platform()})")
    print(f"Python: {platform.python_version()}")
    print(f"seq_bin: {seq_bin}")
    print(f"seq_socket: {seq_socket}")
    print(f"config: {config_path}")
    print(f"iterations={args.iterations}, warmup={args.warmup}, sleep_ms={args.sleep_ms}")
    if macros:
        print(f"macros({len(macros)}): {', '.join(macros)}")
    else:
        print("macros: none (use --macro or ensure config has seqSocket entries)")
    print()

    results: list[ScenarioResult] = []
    resources_to_close: list[Callable[[], None]] = []

    try:
        if os.path.isfile(seq_bin):
            results.append(
                run_scenario(
                    name="seq_cli_ping",
                    iterations=args.iterations,
                    warmup=args.warmup,
                    sleep_ms=args.sleep_ms,
                    probe=make_subprocess_probe([seq_bin, "ping"], timeout_s),
                    notes="spawn seq process each iteration",
                )
            )

            if args.include_open_app:
                app = args.app.replace('"', '\\"')
                legacy_shell = (
                    f'if [ -x "{seq_bin}" ]; then '
                    f'"{seq_bin}" open-app-toggle "{app}" >/dev/null 2>&1 && exit 0; '
                    f'fi; open -a "{app}"'
                )
                results.append(
                    run_scenario(
                        name="legacy_shell_open_app_toggle",
                        iterations=args.iterations,
                        warmup=args.warmup,
                        sleep_ms=args.sleep_ms,
                        probe=make_shell_probe(legacy_shell, timeout_s),
                        notes="legacy /bin/sh + seq path",
                    )
                )
                results.append(
                    run_scenario(
                        name="seq_cli_open_app_toggle",
                        iterations=args.iterations,
                        warmup=args.warmup,
                        sleep_ms=args.sleep_ms,
                        probe=make_subprocess_probe([seq_bin, "open-app-toggle", args.app], timeout_s),
                        notes="spawn seq process each iteration",
                    )
                )

            for macro in macros:
                results.append(
                    run_scenario(
                        name=f"seq_cli_run:{macro}",
                        iterations=args.iterations,
                        warmup=args.warmup,
                        sleep_ms=args.sleep_ms,
                        probe=make_subprocess_probe([seq_bin, "run", macro], timeout_s),
                        notes="spawn seq process each iteration",
                    )
                )
        else:
            print(f"skip: seq_bin not found at {seq_bin}")

        payload = b"RUN ping\n"
        if macros:
            payload = f"RUN {macros[0]}\n".encode("utf-8")
        elif args.include_open_app:
            payload = f"RUN open-app-toggle:{args.app}\n".encode("utf-8")

        if os.path.exists(seq_socket):
            results.append(
                run_scenario(
                    name="socket_stream_connect_per_call",
                    iterations=args.iterations,
                    warmup=args.warmup,
                    sleep_ms=args.sleep_ms,
                    probe=make_stream_probe_connect_per_call(seq_socket, payload, timeout_s),
                    notes="unix stream socket connect/send/close each iteration",
                )
            )

            persistent_probe, sender = make_stream_probe_persistent(seq_socket, payload, timeout_s)
            resources_to_close.append(sender.close)
            results.append(
                run_scenario(
                    name="socket_stream_persistent",
                    iterations=args.iterations,
                    warmup=args.warmup,
                    sleep_ms=args.sleep_ms,
                    probe=persistent_probe,
                    notes="unix stream persistent connection with reconnect",
                )
            )

            dgram_path = seq_socket + ".dgram"
            if os.path.exists(dgram_path):
                dgram_probe, dgram_sock = make_dgram_probe(seq_socket, payload, timeout_s)
                resources_to_close.append(dgram_sock.close)
                results.append(
                    run_scenario(
                        name="socket_dgram_sendto",
                        iterations=args.iterations,
                        warmup=args.warmup,
                        sleep_ms=args.sleep_ms,
                        probe=dgram_probe,
                        notes="unix datagram fire-and-forget",
                    )
                )
            else:
                print(f"skip: datagram socket not found at {dgram_path}")
        else:
            print(f"skip: seq socket not found at {seq_socket}")

    finally:
        for closer in resources_to_close:
            try:
                closer()
            except Exception:
                pass

    if not results:
        print("No scenarios executed.")
        return 1

    print_summary(results)

    if args.json_out:
        out_path = Path(os.path.expanduser(args.json_out))
        out_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "timestamp_unix_s": int(time.time()),
            "host": platform.node(),
            "platform": platform.platform(),
            "python": platform.python_version(),
            "iterations": args.iterations,
            "warmup": args.warmup,
            "sleep_ms": args.sleep_ms,
            "seq_bin": seq_bin,
            "seq_socket": seq_socket,
            "config": str(config_path),
            "macros": macros,
            "results": [r.summary() for r in results],
        }
        out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        print(f"\nWrote {out_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
