#!/usr/bin/env python3
import csv
import math
import os
import sys
from typing import List, Dict, Tuple, Optional

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import Rectangle
import numpy as np
from scipy.spatial import ConvexHull


def read_csv(path: str) -> Tuple[List[str], List[Dict[str, str]]]:
    rows: List[Dict[str, str]] = []
    with open(path, 'r', newline='') as f:
        reader = csv.DictReader(f)
        headers = reader.fieldnames or []
        for r in reader:
            rows.append(r)
    return headers, rows


def to_float(s: str, default: float = float('nan')) -> float:
    if s is None:
        return default
    s = s.strip()
    if s.lower() in ('inf', '+inf'):
        return float('inf')
    if s.lower() in ('-inf'):
        return float('-inf')
    if s.lower() in ('nan', ''):
        return default
    try:
        return float(s)
    except Exception:
        return default


def ensure_dir(path: str):
    os.makedirs(path, exist_ok=True)


def compute_pareto_frontier(x: np.ndarray, y: np.ndarray) -> np.ndarray:
    """
    Compute Pareto frontier for 2D data where we want to maximize both x and y.
    Returns indices of Pareto-optimal points.
    """
    # Filter out NaN/inf values
    valid = ~(np.isnan(x) | np.isnan(y) | np.isinf(x) | np.isinf(y))
    if not np.any(valid):
        return np.array([], dtype=int)
    
    valid_indices = np.where(valid)[0]
    x_valid = x[valid]
    y_valid = y[valid]
    
    # Sort by x descending
    sorted_idx = np.argsort(-x_valid)
    pareto_indices = []
    max_y = -np.inf
    
    for idx in sorted_idx:
        if y_valid[idx] >= max_y:
            pareto_indices.append(valid_indices[idx])
            max_y = y_valid[idx]
    
    return np.array(pareto_indices)


def get_custom_colormap():
    """Returns custom yellow-to-red colormap for time visualization."""
    colors = ['#FFFF00', '#FFA500', '#FF4500', '#DC143C', '#8B0000']
    return LinearSegmentedColormap.from_list('yellow_red', colors)


def make_sorted_table_topN_png(headers: List[str], rows: List[Dict[str, str]], out_png: str, topn: int = 20):
    # Sort by AvgRecall desc, then by AvgAF asc as tie-breaker if present
    def keyfun(r):
        return (-to_float(r.get('AvgRecall', '0'), 0.0), to_float(r.get('AvgAF', 'inf'), float('inf')))

    sorted_rows = sorted(rows, key=keyfun)[:topn]

    # Build table data with all columns
    table_headers = headers
    table_data = [[r.get(h, '') for h in table_headers] for r in sorted_rows]

    # Figure size: width ~ len(headers)*2, height ~ rows*0.35
    n_rows = len(table_data)
    n_cols = len(table_headers)
    fig_w = max(8, min(0.9 * n_cols * 2.0, 40))
    fig_h = max(6, min(0.35 * n_rows + 1.5, 40))

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    ax.axis('off')
    tbl = ax.table(cellText=table_data, colLabels=table_headers, loc='center')
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(8)
    tbl.scale(1, 1.2)
    plt.tight_layout()
    fig.savefig(out_png, dpi=200)
    plt.close(fig)


def make_param_scatter_per_param_all(headers: List[str], rows: List[Dict[str, str]], out_dir: str, base: str):
    # Parameter columns are those before AvgRecall
    if 'AvgRecall' not in headers:
        return
    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]

    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    
    cmap = get_custom_colormap()
    
    # Normalize color range: 0 to max(ttrue) to emphasize when tapprox >= ttrue
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 and not np.all(np.isnan(ttrue)) else np.nanmax(tapprox)

    for pcol in param_cols:
        x = np.array([to_float(r.get(pcol, 'nan')) for r in rows])
        fig, ax = plt.subplots(figsize=(7, 5))
        sc = ax.scatter(x, recall, c=tapprox, cmap=cmap, edgecolors='k', s=50, vmin=0, vmax=vmax)
        ax.set_xlabel(pcol)
        ax.set_ylabel('AvgRecall')
        ax.set_title(f'Recall vs {pcol} (color=AvgTApprox)')
        cbar = fig.colorbar(sc, ax=ax)
        cbar.set_label('AvgTApprox (s)')
        plt.tight_layout()
        out_png = os.path.join(out_dir, f"{base}_recall_vs_{pcol}.png")
        fig.savefig(out_png, dpi=200)
        plt.close(fig)


def make_recall_vs_qps_pareto(rows: List[Dict[str, str]], out_png: str):
    """Recall vs QPS with Pareto frontier."""
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    qps = np.array([to_float(r.get('QPS', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    
    cmap = get_custom_colormap()
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 and not np.all(np.isnan(ttrue)) else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(7, 5))
    sc = ax.scatter(qps, recall, c=tapprox, cmap=cmap, edgecolors='k', s=60, vmin=0, vmax=vmax, alpha=0.7)
    
    # Compute Pareto frontier (maximize both QPS and recall)
    pareto_indices = compute_pareto_frontier(qps, recall)
    if len(pareto_indices) > 0:
        pareto_qps = qps[pareto_indices]
        pareto_recall = recall[pareto_indices]
        sort_idx = np.argsort(pareto_qps)
        ax.plot(pareto_qps[sort_idx], pareto_recall[sort_idx], 'b--', linewidth=2, label='Pareto Frontier')
        ax.scatter(pareto_qps, pareto_recall, s=120, facecolors='none', edgecolors='blue', linewidths=2.5, zorder=10)
    
    ax.set_xlabel('QPS (Queries Per Second)')
    ax.set_ylabel('AvgRecall')
    ax.set_title('Recall vs QPS (Pareto Frontier)')
    ax.legend(loc='best')
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    plt.tight_layout()
    fig.savefig(out_png, dpi=200)
    plt.close(fig)


def make_recall_vs_speedup(rows: List[Dict[str, str]], out_png: str):
    """Recall vs Speedup Factor (tTrue/tApprox)."""
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    
    # Compute speedup
    speedup = np.where((tapprox > 0) & ~np.isnan(tapprox) & ~np.isnan(ttrue), 
                       ttrue / tapprox, np.nan)
    
    cmap = get_custom_colormap()
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 and not np.all(np.isnan(ttrue)) else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(7, 5))
    sc = ax.scatter(speedup, recall, c=tapprox, cmap=cmap, edgecolors='k', s=60, vmin=0, vmax=vmax, alpha=0.7)
    
    # Compute Pareto frontier (maximize both speedup and recall)
    pareto_indices = compute_pareto_frontier(speedup, recall)
    if len(pareto_indices) > 0:
        pareto_speedup = speedup[pareto_indices]
        pareto_recall = recall[pareto_indices]
        sort_idx = np.argsort(pareto_speedup)
        ax.plot(pareto_speedup[sort_idx], pareto_recall[sort_idx], 'b--', linewidth=2, label='Pareto Frontier')
        ax.scatter(pareto_speedup, pareto_recall, s=120, facecolors='none', edgecolors='blue', linewidths=2.5, zorder=10)
    
    ax.set_xlabel('Speedup Factor (tTrue / tApprox)')
    ax.set_ylabel('AvgRecall')
    ax.set_title('Recall vs Speedup Factor (Pareto Frontier)')
    ax.legend(loc='best')
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    plt.tight_layout()
    fig.savefig(out_png, dpi=200)
    plt.close(fig)


def make_parameter_heatmaps(headers: List[str], rows: List[Dict[str, str]], out_dir: str, base: str):
    """Create 2D heatmaps for key parameter pairs."""
    if 'AvgRecall' not in headers:
        return
    
    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]
    
    # Only create heatmaps if we have at least 2 parameters
    if len(param_cols) < 2:
        return
    
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    
    # Generate heatmaps for each pair of parameters
    for i, p1 in enumerate(param_cols):
        for p2 in param_cols[i+1:]:
            x_vals = np.array([to_float(r.get(p1, 'nan')) for r in rows])
            y_vals = np.array([to_float(r.get(p2, 'nan')) for r in rows])
            
            # Get unique values for gridding
            x_unique = np.unique(x_vals[~np.isnan(x_vals)])
            y_unique = np.unique(y_vals[~np.isnan(y_vals)])
            
            if len(x_unique) < 2 or len(y_unique) < 2:
                continue
            
            # Create grid
            grid = np.full((len(y_unique), len(x_unique)), np.nan)
            
            # Fill grid with recall values
            for idx, row in enumerate(rows):
                xv = to_float(row.get(p1, 'nan'))
                yv = to_float(row.get(p2, 'nan'))
                rec = recall[idx]
                
                if not np.isnan(xv) and not np.isnan(yv) and not np.isnan(rec):
                    xi = np.where(x_unique == xv)[0]
                    yi = np.where(y_unique == yv)[0]
                    if len(xi) > 0 and len(yi) > 0:
                        grid[yi[0], xi[0]] = rec
            
            # Plot heatmap with blue-green colormap (different from time's yellow-red)
            fig, ax = plt.subplots(figsize=(8, 6))
            im = ax.imshow(grid, cmap='Blues', aspect='auto', origin='lower', 
                          extent=[x_unique.min(), x_unique.max(), y_unique.min(), y_unique.max()])
            
            # Add text annotations
            for yi, yv in enumerate(y_unique):
                for xi, xv in enumerate(x_unique):
                    if not np.isnan(grid[yi, xi]):
                        text_color = 'white' if grid[yi, xi] > 0.5 else 'black'
                        ax.text(xv, yv, f'{grid[yi, xi]:.2f}', 
                               ha='center', va='center', color=text_color, fontsize=8)
            
            ax.set_xlabel(p1)
            ax.set_ylabel(p2)
            ax.set_title(f'Recall Heatmap: {p1} vs {p2}')
            cbar = fig.colorbar(im, ax=ax)
            cbar.set_label('AvgRecall')
            plt.tight_layout()
            
            out_png = os.path.join(out_dir, f"{base}_heatmap_{p1}_vs_{p2}.png")
            fig.savefig(out_png, dpi=200)
            plt.close(fig)


def make_summary_table(headers: List[str], rows: List[Dict[str, str]], out_png: str):
    """Create summary statistics table comparing best configs."""
    if not rows or 'AvgRecall' not in headers:
        return
    
    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]
    
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    qps = np.array([to_float(r.get('QPS', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    speedup = np.where((tapprox > 0) & ~np.isnan(tapprox) & ~np.isnan(ttrue), 
                       ttrue / tapprox, np.nan)
    
    # Find best configurations
    best_recall_idx = np.nanargmax(recall)
    best_qps_idx = np.nanargmax(qps)
    best_speedup_idx = np.nanargmax(speedup)
    
    # Find balanced config (Pareto optimal with best combined score)
    pareto_indices = compute_pareto_frontier(qps, recall)
    if len(pareto_indices) > 0:
        # Among Pareto points, find one with best harmonic mean of normalized recall and QPS
        norm_recall = (recall[pareto_indices] - np.nanmin(recall)) / (np.nanmax(recall) - np.nanmin(recall) + 1e-10)
        norm_qps = (qps[pareto_indices] - np.nanmin(qps)) / (np.nanmax(qps) - np.nanmin(qps) + 1e-10)
        harmonic_mean = 2 * norm_recall * norm_qps / (norm_recall + norm_qps + 1e-10)
        balanced_idx = pareto_indices[np.nanargmax(harmonic_mean)]
    else:
        balanced_idx = best_recall_idx
    
    # Build table
    configs = [
        ('Best Recall', best_recall_idx),
        ('Fastest (QPS)', best_qps_idx),
        ('Best Speedup', best_speedup_idx),
        ('Balanced (Pareto)', balanced_idx)
    ]
    
    table_data = []
    for name, idx in configs:
        row_data = [name]
        # Add parameter values
        for pcol in param_cols:
            row_data.append(rows[idx].get(pcol, ''))
        # Add metrics
        row_data.append(f"{recall[idx]:.3f}")
        row_data.append(f"{qps[idx]:.1f}")
        row_data.append(f"{tapprox[idx]:.4f}")
        row_data.append(f"{speedup[idx]:.1f}x")
        table_data.append(row_data)
    
    table_headers = ['Config'] + param_cols + ['Recall', 'QPS', 'tApprox', 'Speedup']
    
    # Create figure
    fig, ax = plt.subplots(figsize=(max(10, len(table_headers) * 1.2), 4))
    ax.axis('off')
    tbl = ax.table(cellText=table_data, colLabels=table_headers, loc='center', cellLoc='center')
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(9)
    tbl.scale(1, 2)
    
    # Color header row
    for i in range(len(table_headers)):
        tbl[(0, i)].set_facecolor('#4472C4')
        tbl[(0, i)].set_text_props(weight='bold', color='white')
    
    # Color best values in each column
    for col_idx in range(len(param_cols), len(table_headers)):
        tbl[(4, col_idx)].set_facecolor('#FFE699')  # Highlight balanced config
    
    plt.tight_layout()
    fig.savefig(out_png, dpi=200, bbox_inches='tight')
    plt.close(fig)


def make_recall_vs_tapprox_all(rows: List[Dict[str, str]], out_png: str):
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    
    cmap = get_custom_colormap()
    
    # Normalize color range: 0 to max(ttrue) to emphasize when tapprox >= ttrue
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 and not np.all(np.isnan(ttrue)) else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(7, 5))
    sc = ax.scatter(tapprox, recall, c=tapprox, cmap=cmap, edgecolors='k', s=60, vmin=0, vmax=vmax, alpha=0.7)
    
    # Compute and plot Pareto frontier (minimize time, maximize recall)
    # For Pareto: want low tapprox (so use -tapprox) and high recall
    pareto_indices = compute_pareto_frontier(-tapprox, recall)
    if len(pareto_indices) > 0:
        pareto_tapprox = tapprox[pareto_indices]
        pareto_recall = recall[pareto_indices]
        # Sort by tapprox for line plotting
        sort_idx = np.argsort(pareto_tapprox)
        ax.plot(pareto_tapprox[sort_idx], pareto_recall[sort_idx], 'b--', linewidth=2, label='Pareto Frontier')
        ax.scatter(pareto_tapprox, pareto_recall, s=120, facecolors='none', edgecolors='blue', linewidths=2.5, zorder=10)
    
    ax.set_xlabel('AvgTApprox (s)')
    ax.set_ylabel('AvgRecall')
    ax.set_title('Recall vs Approximate Time (Pareto Frontier)')
    ax.legend(loc='best')
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    plt.tight_layout()
    fig.savefig(out_png, dpi=200)
    plt.close(fig)


def process_csv(csv_path: str):
    headers, rows = read_csv(csv_path)
    if not rows:
        print(f"No data rows in {csv_path}")
        return
    out_dir = os.path.dirname(csv_path)
    base = os.path.splitext(os.path.basename(csv_path))[0]

    print(f"\n{'='*60}")
    print(f"Processing: {csv_path}")
    print(f"{'='*60}")

    # 1) Table sorted by recall (Top-20 only)
    table_png = os.path.join(out_dir, f"{base}_sorted_by_recall_top20.png")
    make_sorted_table_topN_png(headers, rows, table_png, topn=20)
    print(f"✓ Saved: {table_png}")

    # 2) Separate images per parameter (ALL points), color by AvgTApprox
    make_param_scatter_per_param_all(headers, rows, out_dir, base)
    print(f"✓ Saved: {base}_recall_vs_<param>.png for each parameter in {out_dir}")

    # 3) Recall vs AvgTApprox with Pareto frontier
    rt_png = os.path.join(out_dir, f"{base}_recall_vs_tapprox_pareto.png")
    make_recall_vs_tapprox_all(rows, rt_png)
    print(f"✓ Saved: {rt_png}")

    # 4) Recall vs QPS with Pareto frontier
    qps_png = os.path.join(out_dir, f"{base}_recall_vs_qps_pareto.png")
    make_recall_vs_qps_pareto(rows, qps_png)
    print(f"✓ Saved: {qps_png}")

    # 5) Recall vs Speedup Factor with Pareto frontier
    speedup_png = os.path.join(out_dir, f"{base}_recall_vs_speedup_pareto.png")
    make_recall_vs_speedup(rows, speedup_png)
    print(f"✓ Saved: {speedup_png}")

    # 6) Parameter heatmaps for all pairs
    make_parameter_heatmaps(headers, rows, out_dir, base)
    print(f"✓ Saved: {base}_heatmap_*.png for parameter pairs in {out_dir}")

    # 7) Summary statistics table
    summary_png = os.path.join(out_dir, f"{base}_summary_table.png")
    make_summary_table(headers, rows, summary_png)
    print(f"✓ Saved: {summary_png}")
    
    print(f"{'='*60}\n")


def main(argv: List[str]):
    targets: List[str]
    if len(argv) > 1:
        targets = argv[1:]
    else:
        # Default known CSVs in this repo
        targets = [
            'runs/hypercube_sift_100_grid.csv',
            'runs/lsh_sift_100/lsh_sift_100_grid.csv',
            'runs/hypercube_mnist_custom_grid.csv',
            'runs/lsh_mnist_100_grid.csv',
        ]
    for path in targets:
        if os.path.exists(path):
            try:
                process_csv(path)
            except Exception as e:
                print(f"Error processing {path}: {e}")
        else:
            print(f"Missing CSV: {path}")


if __name__ == '__main__':
    main(sys.argv)
