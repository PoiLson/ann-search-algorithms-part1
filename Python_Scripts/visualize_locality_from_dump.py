#!/usr/bin/env python3
"""
Visualize LSH locality using the actual hash tables built by the C code.

The C code writes a dump when env var LSH_DUMP_BUCKETS is set to a filepath.
This script parses that dump and the dataset (optional) to assess locality preservation:
- Collision probability vs distance rank percentile (aggregated across L tables)
- Optional scatter (if dims >= 2) using coordinates from dump for intuition

Usage:
  # Build+run C with dump enabled (example)
  #   LSH_DUMP_BUCKETS=lsh_dump.txt make ALGO=lsh INPUT_FILE=... QUERY_FILE=... TYPE=... K=... L=... W=...
  python3 Python_Scripts/visualize_locality_from_dump.py --dump lsh_dump.txt --data Data/EXPERIMENTS/random_3d_int_6000.txt --samples 1000

Notes:
- If --data is omitted, distances are computed from coordinates embedded in the dump (first up to 3 dims only).
  For high-dim data, pass --data to compute proper Euclidean distances.
"""

import argparse
import os
import re
import numpy as np
import matplotlib.pyplot as plt
from typing import Dict, Tuple


def parse_dump(path: str, max_n: int | None = None, max_tables: int | None = None):
    meta = {}
    tables = {}  # table_id -> dict: idx -> (bucket, id, coords)
    coord_dim = 0
    table_count = 0
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line == 'LSH_DUMP':
                continue
            if line.startswith('L '):
                meta['L'] = int(line.split()[1])
            elif line.startswith('K '):
                meta['K'] = int(line.split()[1])
            elif line.startswith('W '):
                meta['W'] = float(line.split()[1])
            elif line.startswith('TABLE_SIZE '):
                meta['TABLE_SIZE'] = int(line.split()[1])
            elif line.startswith('DATASET_SIZE '):
                meta['DATASET_SIZE'] = int(line.split()[1])
            elif line.startswith('DIM '):
                meta['DIM'] = int(line.split()[1])
            elif line.startswith('DATA_TYPE '):
                meta['DATA_TYPE'] = line.split()[1]
            elif line.startswith('TABLE '):
                if max_tables is not None and table_count >= max_tables:
                    # We've parsed enough tables; stop reading further
                    break
                cur = int(line.split()[1])
                tables[cur] = {}
                table_count += 1
            elif line.startswith('IDX '):
                # Format: IDX i BUCKET b ID id COORD c1 c2 c3
                parts = line.split()
                i = int(parts[1])
                if max_n is not None and i >= max_n:
                    # Skip storing entries beyond limit to keep memory bounded
                    continue
                b = int(parts[3])
                idv = int(parts[5])
                coords = []
                if 'COORD' in parts:
                    pos = parts.index('COORD') + 1
                    for p in parts[pos:]:
                        try:
                            coords.append(float(p))
                        except ValueError:
                            pass
                coord_dim = max(coord_dim, len(coords))
                tables[cur][i] = (b, idv, np.array(coords, dtype=np.float32))
    return meta, tables, coord_dim


def read_text_dataset(path: str) -> np.ndarray:
    with open(path, 'r') as f:
        n, d = map(int, f.readline().strip().split())
        data = np.loadtxt(f, dtype=np.float32)
    if data.shape[0] != n or data.shape[1] != d:
        # try skiprows path
        data = np.loadtxt(path, dtype=np.float32, skiprows=1)
    return data


def compute_collision_rates_from_tables(tables: Dict[int, Dict[int, Tuple[int,int,np.ndarray]]], X: np.ndarray, samples: int, seed: int = 1):
    rng = np.random.default_rng(seed)
    N = X.shape[0]
    L = len(tables)

    # Build fast membership: for each table, map (bucket,id) to a boolean mask of points
    keys_per_table = []
    for t in sorted(tables.keys()):
        T = tables[t]
        # Use the ID (q_id) for exact bucket equivalence
        # Missing indices (due to max_n) default to a sentinel that never collides
        ids = np.full(N, -1, dtype=np.int64)
        for i, triple in T.items():
            if i < N:
                ids[i] = triple[1]
        keys_per_table.append(ids)

    idx = rng.choice(N, size=min(samples, N), replace=False)
    bins = np.linspace(0.0, 1.0, 11)
    num = np.zeros(len(bins)-1, dtype=np.int64)
    den = np.zeros(len(bins)-1, dtype=np.int64)

    for a in idx:
        xa = X[a]
        dists = np.linalg.norm(X - xa, axis=1)
        order = np.argsort(dists)
        ranks = np.empty_like(order)
        ranks[order] = np.arange(N)
        frac = ranks / float(N-1)

        collided = np.zeros(N, dtype=bool)
        for ids in keys_per_table:
            collided |= (ids == ids[a])
        collided[a] = False

        inds = np.digitize(frac, bins) - 1
        for b in range(len(bins)-1):
            mask = inds == b
            if not np.any(mask):
                continue
            den[b] += np.count_nonzero(mask)
            num[b] += np.count_nonzero(collided & mask)

    rates = np.divide(num, den, where=(den>0), out=np.zeros_like(num, dtype=float))
    centers = 0.5*(bins[:-1]+bins[1:])
    return centers, rates


def plot_locality(centers, rates, meta, out_prefix: str, scatter_coords=None, table0_ids=None, no_scatter: bool = False):
    fig, axs = plt.subplots(1, 2, figsize=(12,5))
    axs[0].plot(centers*100, rates*100, marker='o')
    axs[0].set_xlabel('Distance rank percentile (%)')
    axs[0].set_ylabel('Collision probability (%)')
    axs[0].set_title(f"Locality (from dump) L={meta.get('L','?')} k={meta.get('K','?')} w={meta.get('W','?')}")
    axs[0].grid(True, alpha=0.3)

    if (not no_scatter) and scatter_coords is not None and scatter_coords.shape[1] >= 2 and table0_ids is not None:
        keys = table0_ids
        uniq, inv = np.unique(keys, return_inverse=True)
        cmap = plt.get_cmap('tab20')
        colors = [cmap(i % 20) for i in inv]
        axs[1].scatter(scatter_coords[:,0], scatter_coords[:,1], c=colors, s=10, alpha=0.85, edgecolor='none')
        axs[1].set_title('Table 0 buckets (from dump)')
        axs[1].set_xlabel('X0')
        axs[1].set_ylabel('X1')
        axs[1].grid(True, alpha=0.3)
    else:
        axs[1].axis('off')

    plt.tight_layout()
    out = f"{out_prefix}_locality_dump.png"
    plt.savefig(out, dpi=200, bbox_inches='tight')
    print(f"Saved locality plot to {out}")


def main():
    ap = argparse.ArgumentParser(description='Locality from C LSH dump')
    ap.add_argument('--dump', required=True, help='Path to LSH_DUMP_BUCKETS output file')
    ap.add_argument('--data', help='Path to dataset (text: first line n d). If omitted, use coords from dump (up to 3 dims).')
    ap.add_argument('--samples', type=int, default=500)
    ap.add_argument('--max-n', type=int, default=None, help='Limit to first N points from dump/data to bound memory and runtime')
    ap.add_argument('--max-tables', type=int, default=None, help='Limit number of tables parsed from dump (e.g., 5 or 10)')
    ap.add_argument('--no-scatter', action='store_true', help='Disable scatter subplot to save time/memory')
    ap.add_argument('--seed', type=int, default=1)
    args = ap.parse_args()

    meta, tables, coord_dim = parse_dump(args.dump, max_n=args.max_n, max_tables=args.max_tables)
    L = meta.get('L', 0)
    print(f"Parsed dump: L={L}, N={meta.get('DATASET_SIZE','?')}, D={meta.get('DIM','?')}")

    # Build X for distance computations
    if args.data:
        # Try to read text dataset; if fvecs, this function won't work—prefer using coords from dump
        X = read_text_dataset(args.data)
        if args.max_n is not None and X.shape[0] > args.max_n:
            X = X[:args.max_n]
    else:
        # Assemble coords from dump (only as many dims as were stored, up to 3)
        # Use table 0 for coordinates (they are identical across tables)
        t0 = tables[min(tables.keys())]
        N = (max(t0.keys()) + 1) if t0 else 0
        if args.max_n is not None:
            N = min(N, args.max_n)
        X = np.zeros((N, coord_dim), dtype=np.float32)
        # Populate only indices present to avoid KeyError
        for i, triple in t0.items():
            if i < N:
                X[i,:] = triple[2][:coord_dim]
        if coord_dim < 2:
            print("Warning: insufficient coordinate dims in dump; scatter will be disabled.")

    centers, rates = compute_collision_rates_from_tables(tables, X, samples=args.samples, seed=args.seed)

    # Prepare optional scatter from dump and table0 ids
    table0 = tables[min(tables.keys())]
    # Build id array length N with default sentinel -1
    t0_ids = np.full(X.shape[0], -1, dtype=np.int64)
    for i, triple in table0.items():
        if i < t0_ids.shape[0]:
            t0_ids[i] = triple[1]
    coords_for_scatter = None
    if coord_dim >= 2:
        coords_for_scatter = np.array([table0[i][2][:2] for i in range(len(table0))], dtype=np.float32)

    out_prefix = os.path.splitext(os.path.basename(args.dump))[0]
    plot_locality(centers, rates, meta, out_prefix, scatter_coords=coords_for_scatter, table0_ids=t0_ids, no_scatter=args.no_scatter)

    # Print a compact summary
    print(f"Closest decile collision rate: {rates[0]*100:.2f}% | farthest decile: {rates[-1]*100:.2f}%")


if __name__ == '__main__':
    main()
