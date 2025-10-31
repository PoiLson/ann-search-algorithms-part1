#!/usr/bin/env python3
import argparse
import csv
import os
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

METRICS_HDR_RE = re.compile(r"^===== OVERALL METRICS =====\s*$", re.MULTILINE)
FLOAT_RE = r"([-+]?\d*\.?\d+(?:[eE][-+]?\d+)?)"
AF_RE = re.compile(rf"Average AF.*?:\s*{FLOAT_RE}")
RECALL_RE = re.compile(rf"Average Recall@N:\s*{FLOAT_RE}")
TAPPROX_RE = re.compile(rf"Average tApproximate:\s*{FLOAT_RE}")
TTRUE_RE = re.compile(rf"Average tTrue:\s*{FLOAT_RE}")
QPS_RE = re.compile(rf"QPS_overall:\s*{FLOAT_RE}")
ALGOS = ("LSH", "Hypercube", "IVFFlat", "IVFPQ")
NAME_RE = re.compile(r"run_k(?P<k>\d+)_L(?P<L>\d+)_w(?P<w>\d+)\.")


def find_files(inputs: List[str]) -> List[Path]:
    files: List[Path] = []
    for inp in inputs:
        p = Path(inp)
        if p.is_dir():
            for fp in p.rglob("*.txt"):
                files.append(fp)
        elif p.is_file():
            files.append(p)
        else:
            # Glob pattern support
            for fp in Path().glob(inp):
                if fp.is_file():
                    files.append(fp)
    # de-duplicate while keeping order
    seen = set()
    uniq: List[Path] = []
    for f in files:
        if f.resolve() not in seen:
            seen.add(f.resolve())
            uniq.append(f)
    return uniq


def parse_algo(text: str) -> Optional[str]:
    # Return the first known algo token if present near file start
    head = text[:2048]
    for a in ALGOS:
        if re.search(rf"^\s*{a}\s*$", head, re.MULTILINE):
            return a
    return None


def parse_params_from_name(name: str) -> Tuple[Optional[int], Optional[int], Optional[int]]:
    m = NAME_RE.search(name)
    if not m:
        return None, None, None
    return int(m.group("k")), int(m.group("L")), int(m.group("w"))


def extract_metrics(text: str) -> Optional[Dict[str, float]]:
    m = METRICS_HDR_RE.search(text)
    if not m:
        return None
    tail = text[m.end():]
    def pick(rx: re.Pattern) -> Optional[float]:
        mm = rx.search(tail)
        return float(mm.group(1)) if mm else None
    af = pick(AF_RE)
    rec = pick(RECALL_RE)
    tapp = pick(TAPPROX_RE)
    ttrue = pick(TTRUE_RE)
    qps = pick(QPS_RE)
    if any(v is None for v in (af, rec, tapp, ttrue, qps)):
        return None
    return {"AF": af, "Recall": rec, "tApprox": tapp, "tTrue": ttrue, "QPS": qps}


def main():
    ap = argparse.ArgumentParser(description="Extract OVERALL METRICS blocks into a CSV")
    ap.add_argument("inputs", nargs="*", default=["runs"], help="Input files/dirs/globs to scan (default: runs)")
    ap.add_argument("--output", "-o", default="runs/overall_metrics.csv", help="Output CSV path")
    args = ap.parse_args()

    paths = find_files(args.inputs)
    rows: List[Dict[str, object]] = []

    for fp in paths:
        try:
            text = fp.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        metrics = extract_metrics(text)
        if not metrics:
            continue
        algo = parse_algo(text) or ""
        k, L, w = parse_params_from_name(fp.name)
        rows.append({
            "file": str(fp),
            "algorithm": algo,
            "k": k if k is not None else "",
            "L": L if L is not None else "",
            "w": w if w is not None else "",
            **metrics,
        })

    os.makedirs(Path(args.output).parent, exist_ok=True)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "file", "algorithm", "k", "L", "w", "AF", "Recall", "tApprox", "tTrue", "QPS"
        ])
        writer.writeheader()
        for r in rows:
            writer.writerow(r)

    print(f"Wrote {len(rows)} rows to {args.output}")


if __name__ == "__main__":
    main()
