#!/usr/bin/env python3

import argparse
import json
import math
import os
from pathlib import Path


def load(path: str) -> dict:
    p = Path(os.path.expanduser(path))
    return json.loads(p.read_text(encoding="utf-8"))


def by_name(payload: dict) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for row in payload.get("results", []):
        name = row.get("name")
        if isinstance(name, str):
            out[name] = row
    return out


def pct_delta(base: float, cand: float) -> float:
    if base == 0:
        return math.inf
    return ((cand - base) / base) * 100.0


def fmt_num(v) -> str:
    if v is None:
        return "-"
    if isinstance(v, (int, float)):
        return f"{v:.3f}"
    return str(v)


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare two bench_seq_latency JSON outputs.")
    parser.add_argument("--baseline", required=True)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--metric", default="p95_us", choices=["p50_us", "p90_us", "p95_us", "p99_us", "mean_us"])
    args = parser.parse_args()

    baseline = load(args.baseline)
    candidate = load(args.candidate)

    b = by_name(baseline)
    c = by_name(candidate)
    names = sorted(set(b.keys()) & set(c.keys()))

    if not names:
        print("No overlapping scenarios found.")
        return 1

    header = ["Scenario", f"base:{args.metric}", f"cand:{args.metric}", "delta(us)", "delta(%)"]
    rows = [header]

    for name in names:
        bv = b[name].get(args.metric)
        cv = c[name].get(args.metric)
        if not isinstance(bv, (int, float)) or not isinstance(cv, (int, float)):
            rows.append([name, fmt_num(bv), fmt_num(cv), "-", "-"])
            continue

        delta_us = cv - bv
        delta_pct = pct_delta(float(bv), float(cv))
        rows.append(
            [
                name,
                fmt_num(bv),
                fmt_num(cv),
                f"{delta_us:.3f}",
                f"{delta_pct:.2f}%",
            ]
        )

    widths = [max(len(row[i]) for row in rows) for i in range(len(header))]
    for idx, row in enumerate(rows):
        line = "  ".join(row[i].ljust(widths[i]) for i in range(len(header)))
        print(line)
        if idx == 0:
            print("  ".join("-" * w for w in widths))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
