#!/usr/bin/env python3
"""
Visualize LSH locality preservation on a dataset.

- Reads a text dataset with header: "<size> <dimension>" then size lines of coordinates
- Implements a simple E2LSH (L tables optional; default L=1) in Python
- Shows whether nearby points tend to land in the same hash bucket (collision rate vs distance)
- Plots the point cloud (2D/3D) colored by bucket for intuition

Usage examples:
  python visualize_locality.py --data Data/EXPERIMENTS/random_3d_int_6000.txt --k 4 --w 30 --L 1 --samples 500
  python visualize_locality.py --data Data/EXPERIMENTS/random_3d_points.txt --k 6 --w 5 --L 2 --samples 500
"""

import argparse
import math
import os
import sys
from typing import Tuple

import numpy as np
import matplotlib.pyplot as plt


def read_text_dataset(path: str) -> np.ndarray:
    """Read dataset with first line "size dim" and return np.ndarray of shape (size, dim) dtype=float32."""
    with open(path, 'r') as f:
        header = f.readline().strip().split()
        if len(header) < 2:
            raise ValueError("Invalid header: expected '<size> <dimension>'")
        n, d = int(header[0]), int(header[1])
        data = np.loadtxt(f, dtype=np.float32)
        if data.shape != (n, d):
            # Some writers include header in the same stream; if mismatch, try reading entire file
            f.seek(0)
            raw = np.loadtxt(path, dtype=np.float32, skiprows=1)
            if raw.shape != (n, d):
                raise ValueError(f"Data shape mismatch: header says {(n,d)}, file has {raw.shape}")
            data = raw
    return data


def e2lsh_hash(X: np.ndarray, k: int, w: float, L: int, rng: np.random.Generator) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Compute E2LSH hashes for X.
    Returns:
      h_all: shape (L, N, k) integer h_i values per table
      v_all: shape (L, k, d) projection vectors (for reproducibility)
      t_all: shape (L, k) shift values in [0, w)
    """
    N, d = X.shape
    v_all = rng.normal(size=(L, k, d)).astype(np.float32)
    t_all = rng.uniform(0.0, w, size=(L, k)).astype(np.float32)

    # Compute projections per table: for each l, compute X @ v[l].T -> (N,k)
    h_all = np.empty((L, N, k), dtype=np.int64)
    for l in range(L):
        proj = X @ v_all[l].T  # (N,k)
        val = (proj + t_all[l]) / float(w)
        h = np.floor(val).astype(np.int64)
        h_all[l] = h
    return h_all, v_all, t_all


def g_tuple(h: np.ndarray) -> np.ndarray:
    """Convert h of shape (N,k) to a single bucket key using view-as-void for grouping tuples."""
    # Pack k int64s per row into a bytes view for hashable grouping
    return h.view(np.dtype((np.void, h.dtype.itemsize * h.shape[1]))).ravel()


def compute_collision_vs_distance(X: np.ndarray, h_all: np.ndarray, samples: int, L: int, rng: np.random.Generator):
    """Compute collision rate vs Euclidean distance percentiles using sampled anchors.
    Returns:
      centers: bin centers
      rates: collision probability per bin
    """
    N = X.shape[0]
    idx = rng.choice(N, size=min(samples, N), replace=False)

    # Precompute norms for fast distances
    # We'll compute distances anchor-wise to all points
    bins = np.linspace(0.0, 1.0, 11)  # deciles by distance rank percentile
    num = np.zeros(len(bins) - 1, dtype=np.int64)
    den = np.zeros(len(bins) - 1, dtype=np.int64)

    for a in idx:
        xa = X[a]
        dists = np.linalg.norm(X - xa, axis=1)
        order = np.argsort(dists)
        ranks = np.empty_like(order)
        ranks[order] = np.arange(N)
        # Normalize rank to [0,1]
        frac = ranks / float(N - 1)

        # Collision if any table places them in the same g (k-tuple)
        collided = np.zeros(N, dtype=bool)
        for l in range(L):
            same = g_tuple(h_all[l, a:a+1]) == g_tuple(h_all[l])
            collided |= same
        # Exclude self
        collided[a] = False

        # Bin by fractional rank
        inds = np.digitize(frac, bins) - 1
        for b in range(len(bins) - 1):
            mask = inds == b
            if not np.any(mask):
                continue
            den[b] += np.count_nonzero(mask)
            num[b] += np.count_nonzero(collided & mask)

    rates = np.divide(num, den, where=(den > 0), out=np.zeros_like(num, dtype=float))
    centers = 0.5 * (bins[:-1] + bins[1:])
    return centers, rates


def plot_locality(X: np.ndarray, h_all: np.ndarray, centers, rates, L: int, k: int, w: float, out_prefix: str):
    """Create plots: collision rate vs distance rank; 2D/3D scatter colored by bucket of table 0."""
    fig, axs = plt.subplots(1, 2, figsize=(12, 5))

    # Left: Collision probability vs distance percentile
    axs[0].plot(centers * 100, rates * 100, marker='o')
    axs[0].set_xlabel('Distance rank percentile (%)')
    axs[0].set_ylabel('Collision probability (%)')
    axs[0].set_title(f'Locality preservation (L={L}, k={k}, w={w})')
    axs[0].grid(True, alpha=0.3)

    # Right: Scatter colored by bucket in table 0 (use first 2 dims)
    h0 = h_all[0]
    keys0 = g_tuple(h0)
    # Assign a color per unique key (cap number of colors for clarity)
    uniq, inv = np.unique(keys0, return_inverse=True)
    # Reduce number of colors if many buckets
    cmap = plt.get_cmap('tab20')
    colors = [cmap(i % 20) for i in inv]

    if X.shape[1] >= 2:
        axs[1].scatter(X[:, 0], X[:, 1], c=colors, s=10, alpha=0.8, edgecolor='none')
        axs[1].set_xlabel('X0')
        axs[1].set_ylabel('X1')
    else:
        axs[1].scatter(np.arange(X.shape[0]), X[:, 0], c=colors, s=10, alpha=0.8, edgecolor='none')
        axs[1].set_xlabel('index')
        axs[1].set_ylabel('X0')
    axs[1].set_title('Points colored by bucket (table 0)')
    axs[1].grid(True, alpha=0.3)

    plt.tight_layout()
    out = f"{out_prefix}_locality.png"
    plt.savefig(out, dpi=200, bbox_inches='tight')
    print(f"Saved locality plot to {out}")


def main():
    ap = argparse.ArgumentParser(description='Visualize LSH locality preservation')
    ap.add_argument('--data', required=True, help='Path to text dataset (first line: size dim)')
    ap.add_argument('--k', type=int, default=4, help='Number of hash functions per table')
    ap.add_argument('--w', type=float, default=30.0, help='Bucket width w')
    ap.add_argument('--L', type=int, default=1, help='Number of hash tables')
    ap.add_argument('--samples', type=int, default=500, help='Number of anchor points to sample')
    ap.add_argument('--seed', type=int, default=1, help='Random seed')
    args = ap.parse_args()

    rng = np.random.default_rng(args.seed)

    print(f"Loading dataset: {args.data}")
    X = read_text_dataset(args.data)
    print(f"Loaded X: {X.shape}, dtype={X.dtype}")

    # Normalize scale optionally? For raw ints 0-255, E2LSH with moderate w works.
    print(f"Hashing with E2LSH: L={args.L}, k={args.k}, w={args.w}")
    h_all, v_all, t_all = e2lsh_hash(X.astype(np.float32), k=args.k, w=args.w, L=args.L, rng=rng)

    print("Computing collision probability vs distance percentile...")
    centers, rates = compute_collision_vs_distance(X.astype(np.float32), h_all, samples=args.samples, L=args.L, rng=rng)

    out_prefix = os.path.splitext(os.path.basename(args.data))[0]
    plot_locality(X, h_all, centers, rates, args.L, args.k, args.w, out_prefix)

    # Print a simple locality score: collision among top-1% vs last-1%
    if len(centers) >= 2:
        lo = rates[0]
        hi = rates[-1]
        print(f"Collision rate near (closest decile): {lo*100:.2f}% | farthest decile: {hi*100:.2f}%")


if __name__ == '__main__':
    main()
