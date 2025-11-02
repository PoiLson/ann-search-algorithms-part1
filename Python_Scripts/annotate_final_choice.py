#!/usr/bin/env python3
"""
Script to annotate final chosen configurations on existing plots.
Usage: python3 annotate_final_choice.py <csv_path> <row_index>

Example:
  python3 annotate_final_choice.py runs/lsh_mnist_100_grid.csv 42

This will:
1. Read the CSV and identify row 42 as your final choice
2. Regenerate all Pareto frontier plots with this configuration highlighted
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


def smart_annotation_position(x_pos, y_pos, x_range, y_range, legend_pos: str | None = None, has_right_cbar: bool = True):
    """Pick a text offset (dx, dy) that avoids covering title, legend, colorbar, and edges.

    Heuristics:
    - If point is on the right, place text left; otherwise place right.
    - If point is near the top, place text below; if near the bottom, place above.
    - Nudge away from the legend corner based on legend_pos.
    """
    xmin, xmax = x_range
    ymin, ymax = y_range
    xw = max(1e-12, xmax - xmin)
    yw = max(1e-12, ymax - ymin)
    x_frac = (x_pos - xmin) / xw
    y_frac = (y_pos - ymin) / yw

    # Base offsets: left/right and up/down
    if x_frac > 0.8 and has_right_cbar:
        dx = -220  # far left to clear colorbar
    elif x_frac > 0.7:
        dx = -160
    elif x_frac > 0.65:
        dx = -120
    else:
        dx = 20
    # Avoid top title area and bottom legends/labels
    if y_frac > 0.7:
        dy = -40   # push downward if high
    elif y_frac < 0.3:
        dy = 30    # push upward if low
    else:
        dy = 20    # slight upward bias

    # Extra nudge away from typical legend location
    if legend_pos == 'lower left' and x_frac < 0.4 and y_frac < 0.4:
        dx, dy = (20, 40)
    elif legend_pos == 'lower right' and x_frac > 0.6 and y_frac < 0.4:
        dx, dy = (-120, 40)

    return dx, dy


def smart_axes_anchor(x_pos, y_pos, ax, legend_pos: str | None = None, avoid_right_cbar: bool = True):
    """Return a safe axes-fraction anchor (x,y) for the annotation plus ha/va.

    This guarantees the annotation box is drawn inside the axes, avoiding the colorbar
    on the right and staying clear of the title/top and legend corners.
    """
    xmin, xmax = ax.get_xlim()
    ymin, ymax = ax.get_ylim()
    xw = max(1e-12, xmax - xmin)
    yw = max(1e-12, ymax - ymin)
    x_frac = (x_pos - xmin) / xw
    y_frac = (y_pos - ymin) / yw

    # Choose side: if point is on right, place text on left half; else right half
    x_text = 0.18 if (avoid_right_cbar and x_frac > 0.6) else 0.82

    # Vertical placement: nudge away from edges and legend
    if y_frac > 0.75:
        y_text = 0.6
        va = 'top'
    elif y_frac < 0.25:
        y_text = 0.4
        va = 'bottom'
    else:
        y_text = 0.7
        va = 'top'

    # Avoid legend corners
    if legend_pos == 'lower left' and x_text < 0.35 and y_text < 0.35:
        y_text = 0.55
    if legend_pos == 'lower right' and x_text > 0.65 and y_text < 0.35:
        y_text = 0.55

    # Clip to safe interior band
    x_text = min(max(x_text, 0.12), 0.88)
    y_text = min(max(y_text, 0.2), 0.85)
    ha = 'left' if x_text <= 0.5 else 'right'
    return (x_text, y_text, ha, va)


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
    
    # Determine annotation position to avoid overlap
    x_range = ax.get_xlim()
    y_range = ax.get_ylim()
    x_text, y_text, ha, va = smart_axes_anchor(chosen_qps, chosen_recall, ax, legend_pos='lower left', avoid_right_cbar=True)
    ax.annotate(f'CHOSEN:\n{param_str}\nRecall={chosen_recall:.3f}, QPS={chosen_qps:.1f}',
               xy=(chosen_qps, chosen_recall),
               xytext=(x_text, y_text), textcoords='axes fraction',
               bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.9, edgecolor='black', linewidth=1.5),
               arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0.2', lw=2, color='black'),
               fontsize=9, weight='bold', zorder=20, ha=ha, va=va)
    
    ax.set_xlabel('QPS (Queries Per Second)', fontsize=11)
    ax.set_ylabel('AvgRecall', fontsize=11)
    ax.set_title('Parameter Selection: Recall vs QPS', fontsize=13, weight='bold')
    ax.legend(loc='lower left', fontsize=10)
    ax.grid(True, alpha=0.3)
    
    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    
    plt.tight_layout()
    
    out_dir = os.path.dirname(csv_path)
    base = os.path.splitext(os.path.basename(csv_path))[0]
    out_png = os.path.join(out_dir, f"{base}_FINAL_CHOICE_qps_annotated.png")
    fig.savefig(out_png, dpi=300)
    plt.close(fig)
    
    print(f"✓ Saved annotated plot: {out_png}")
    return out_png


def annotate_recall_vs_speedup(csv_path: str, chosen_idx: int):
    """Create recall vs speedup plot with chosen config highlighted."""
    headers, rows = read_csv(csv_path)
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])

    speedup = np.where((tapprox > 0) & ~np.isnan(tapprox) & ~np.isnan(ttrue),
                       ttrue / tapprox, np.nan)

    cmap = get_custom_colormap()
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(8, 6))
    sc = ax.scatter(speedup, recall, c=tapprox, cmap=cmap, edgecolors='k',
                    s=60, vmin=0, vmax=vmax, alpha=0.6, zorder=1)

    pareto_indices = compute_pareto_frontier(speedup, recall)
    if len(pareto_indices) > 0:
        pareto_speedup = speedup[pareto_indices]
        pareto_recall = recall[pareto_indices]
        sort_idx = np.argsort(pareto_speedup)
        ax.plot(pareto_speedup[sort_idx], pareto_recall[sort_idx], 'b--',
                linewidth=2, label='Pareto Frontier', zorder=5)

    chosen_recall = recall[chosen_idx]
    chosen_speedup = speedup[chosen_idx]
    ax.scatter([chosen_speedup], [chosen_recall], s=300, marker='*',
               c='gold', edgecolors='black', linewidths=2,
               label='Selected Config', zorder=10)

    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]
    param_str = ', '.join([f"{p}={rows[chosen_idx].get(p, '')}" for p in param_cols])

    x_range = ax.get_xlim()
    y_range = ax.get_ylim()
    x_text, y_text, ha, va = smart_axes_anchor(chosen_speedup, chosen_recall, ax, legend_pos='lower right', avoid_right_cbar=True)
    ax.annotate(f'CHOSEN:\n{param_str}\nRecall={chosen_recall:.3f}, Speedup={chosen_speedup:.1f}x',
                xy=(chosen_speedup, chosen_recall),
                xytext=(x_text, y_text), textcoords='axes fraction',
                bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.9, edgecolor='black', linewidth=1.5),
                arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0.2', lw=2, color='black'),
                fontsize=9, weight='bold', zorder=20, ha=ha, va=va)

    ax.set_xlabel('Speedup Factor (tTrue / tApprox)', fontsize=11)
    ax.set_ylabel('AvgRecall', fontsize=11)
    ax.set_title('Parameter Selection: Recall vs Speedup', fontsize=13, weight='bold')
    ax.legend(loc='lower right', fontsize=10)
    ax.grid(True, alpha=0.3)

    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    plt.tight_layout()

    out_dir = os.path.dirname(csv_path)
    base = os.path.splitext(os.path.basename(csv_path))[0]
    out_png = os.path.join(out_dir, f"{base}_FINAL_CHOICE_speedup_annotated.png")
    fig.savefig(out_png, dpi=300)
    plt.close(fig)

    print(f"✓ Saved annotated plot: {out_png}")
    return out_png


def annotate_recall_vs_time(csv_path: str, chosen_idx: int):
    """Create recall vs time plot with chosen config highlighted."""
    headers, rows = read_csv(csv_path)
    recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
    tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
    ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])

    cmap = get_custom_colormap()
    vmax = np.nanmax(ttrue) if len(ttrue) > 0 else np.nanmax(tapprox)

    fig, ax = plt.subplots(figsize=(8, 6))
    sc = ax.scatter(tapprox, recall, c=tapprox, cmap=cmap, edgecolors='k',
                    s=60, vmin=0, vmax=vmax, alpha=0.6, zorder=1)

    pareto_indices = compute_pareto_frontier(-tapprox, recall)
    if len(pareto_indices) > 0:
        pareto_tapprox = tapprox[pareto_indices]
        pareto_recall = recall[pareto_indices]
        sort_idx = np.argsort(pareto_tapprox)
        ax.plot(pareto_tapprox[sort_idx], pareto_recall[sort_idx], 'b--',
                linewidth=2, label='Pareto Frontier', zorder=5)

    chosen_recall = recall[chosen_idx]
    chosen_tapprox = tapprox[chosen_idx]
    ax.scatter([chosen_tapprox], [chosen_recall], s=300, marker='*',
               c='gold', edgecolors='black', linewidths=2,
               label='Selected Config', zorder=10)

    recall_idx = headers.index('AvgRecall')
    param_cols = headers[:recall_idx]
    param_str = ', '.join([f"{p}={rows[chosen_idx].get(p, '')}" for p in param_cols])

    x_range = ax.get_xlim()
    y_range = ax.get_ylim()
    x_text, y_text, ha, va = smart_axes_anchor(chosen_tapprox, chosen_recall, ax, legend_pos='lower right', avoid_right_cbar=True)
    ax.annotate(f'CHOSEN:\n{param_str}\nRecall={chosen_recall:.3f}, Time={chosen_tapprox:.4f}s',
                xy=(chosen_tapprox, chosen_recall),
                xytext=(x_text, y_text), textcoords='axes fraction',
                bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.9, edgecolor='black', linewidth=1.5),
                arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0.2', lw=2, color='black'),
                fontsize=9, weight='bold', zorder=20, ha=ha, va=va)

    ax.set_xlabel('AvgTApprox (s)', fontsize=11)
    ax.set_ylabel('AvgRecall', fontsize=11)
    ax.set_title('Parameter Selection: Recall vs Time', fontsize=13, weight='bold')
    ax.legend(loc='lower right', fontsize=10)
    ax.grid(True, alpha=0.3)

    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label('AvgTApprox (s)')
    plt.tight_layout()

    out_dir = os.path.dirname(csv_path)
    base = os.path.splitext(os.path.basename(csv_path))[0]
    out_png = os.path.join(out_dir, f"{base}_FINAL_CHOICE_time_annotated.png")
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
    
    # Create all annotated Pareto plots
    print("\nGenerating annotated Pareto frontier plots...")
    annotate_recall_vs_qps(csv_path, chosen_idx)
    annotate_recall_vs_speedup(csv_path, chosen_idx)
    annotate_recall_vs_time(csv_path, chosen_idx)

    print("\n✓ Done! All three annotated Pareto plots generated.")
    print("  Use these in your report/presentation.")


if __name__ == '__main__':
    main()
