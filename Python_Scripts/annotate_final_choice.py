#!/usr/bin/env python3
"""
Script to annotate final chosen configurations on existing plots.
Usage: python3 annotate_final_choice.py <csv_path> <row_index>

Example:
  python3 annotate_final_choice.py runs/lsh_mnist_100_grid.csv 42

This will:
1. Read the CSV and identify row 42 as your final choice
2. Regenerate key plots with this configuration highlighted
3. Add annotation showing parameter values
"""

import sys
import csv
import os
from typing import Dict, List
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import numpy as np

# Import helper functions from plot_csv_metrics
sys.path.insert(0, os.path.dirname(__file__))
from plot_csv_metrics import read_csv, to_float, compute_pareto_frontier, get_custom_colormap


def annotate_recall_vs_qps(csv_path: str, chosen_idx: int):
    """Create recall vs QPS plot with chosen config highlighted."""
    headers, rows = read_csv(csv_path)
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    qps = np.array([to_float(r.get('QPS', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
    
    cmap = get_custom_colormap()
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(8, 6))
    
    # Plot all points
    sc = ax.scatter(qps, recall, c=tapprox, cmap=cmap, edgecolors='k', 
                   s=60, vmin=0, vmax=vmax, alpha=0.6, zorder=1)
    
    # Plot Pareto frontier
    pareto_indices = compute_pareto_frontier(qps, recall)
    if len(pareto_indices) > 0:
        pareto_qps = qps[pareto_indices]
        pareto_recall = recall[pareto_indices]
        sort_idx = np.argsort(pareto_qps)
        ax.plot(pareto_qps[sort_idx], pareto_recall[sort_idx], 'b--', 
               linewidth=2, label='Pareto Frontier', zorder=5)
    
    # Highlight chosen configuration
    chosen_recall = recall[chosen_idx]
    chosen_qps = qps[chosen_idx]
    ax.scatter([chosen_qps], [chosen_recall], s=300, marker='*', 
              c='gold', edgecolors='black', linewidths=2, 
              label='Selected Config', zorder=10)
    
    # Add annotation with parameter values
    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]
    param_str = ', '.join([f"{p}={rows[chosen_idx].get(p, '')}" for p in param_cols])
    
    ax.annotate(f'CHOSEN:\n{param_str}\nRecall={chosen_recall:.3f}, QPS={chosen_qps:.1f}',
               xy=(chosen_qps, chosen_recall),
               xytext=(20, 20), textcoords='offset points',
               bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.8),
               arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0', lw=2),
               fontsize=9, weight='bold')
    
    ax.set_xlabel('QPS (Queries Per Second)', fontsize=11)
    ax.set_ylabel('AvgRecall', fontsize=11)
    ax.set_title('Parameter Selection: Recall vs QPS', fontsize=13, weight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    
    plt.tight_layout()
    
    out_dir = os.path.dirname(csv_path)
    base = os.path.splitext(os.path.basename(csv_path))[0]
    out_png = os.path.join(out_dir, f"{base}_FINAL_CHOICE_annotated.png")
    fig.savefig(out_png, dpi=300)
    plt.close(fig)
    
    print(f"✓ Saved annotated plot: {out_png}")
    return out_png


def print_chosen_config(csv_path: str, chosen_idx: int):
    """Print details of chosen configuration."""
    headers, rows = read_csv(csv_path)
    
    print(f"\n{'='*70}")
    print(f"FINAL CHOSEN CONFIGURATION (Row {chosen_idx})")
    print(f"{'='*70}")
    
    row = rows[chosen_idx]
    
    # Print parameters
    recall_idx = headers.index('AvgRecall') if 'AvgRecall' in headers else len(headers)
    param_cols = headers[:recall_idx]
    
    print("\nParameters:")
    for p in param_cols:
        print(f"  {p:15s} = {row.get(p, 'N/A')}")
    
    # Print metrics
    print("\nPerformance Metrics:")
    recall = to_float(row.get('AvgRecall', 'nan'))
    af = to_float(row.get('AvgAF', 'nan'))
    tapprox = to_float(row.get('AvgTApprox', 'nan'))
    ttrue = to_float(row.get('AvgTTrue', 'nan'))
    qps = to_float(row.get('QPS', 'nan'))
    speedup = ttrue / tapprox if tapprox > 0 else float('nan')
    
    print(f"  {'Recall':15s} = {recall:.4f} ({recall*100:.2f}%)")
    print(f"  {'QPS':15s} = {qps:.2f} queries/second")
    print(f"  {'AvgTApprox':15s} = {tapprox:.6f} seconds")
    print(f"  {'AvgTTrue':15s} = {ttrue:.6f} seconds")
    print(f"  {'Speedup':15s} = {speedup:.2f}x faster than brute-force")
    print(f"  {'AvgAF':15s} = {af:.4f}")
    
    print(f"\n{'='*70}\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 annotate_final_choice.py <csv_path> <row_index>")
        print("\nExample:")
        print("  python3 annotate_final_choice.py runs/lsh_mnist_100_grid.csv 42")
        print("\nTo find the row index, open the CSV and count rows (0-indexed, excluding header)")
        sys.exit(1)
    
    csv_path = sys.argv[1]
    
    if not os.path.exists(csv_path):
        print(f"Error: CSV file not found: {csv_path}")
        sys.exit(1)
    
    try:
        chosen_idx = int(sys.argv[2])
    except ValueError:
        print(f"Error: Row index must be an integer, got: {sys.argv[2]}")
        sys.exit(1)
    
    headers, rows = read_csv(csv_path)
    
    if chosen_idx < 0 or chosen_idx >= len(rows):
        print(f"Error: Row index {chosen_idx} out of range (0-{len(rows)-1})")
        sys.exit(1)
    
    # Print configuration details
    print_chosen_config(csv_path, chosen_idx)
    
    # Create annotated plot
    annotate_recall_vs_qps(csv_path, chosen_idx)
    
    print("✓ Done! Use this annotated plot in your report/presentation.")


if __name__ == '__main__':
    main()
