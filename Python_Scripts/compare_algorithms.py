#!/usr/bin/env python3
"""
Create cross-algorithm comparison plots for MNIST and SIFT datasets.
Shows how LSH and Hypercube compare on the same dataset.
"""

import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
from plot_csv_metrics import read_csv, to_float, compute_pareto_frontier, get_custom_colormap


def compare_algorithms(csv_paths: dict, dataset_name: str, out_path: str):
    """
    Create comparison plots for multiple algorithms on same dataset.
    
    Args:
        csv_paths: dict mapping algorithm name to CSV path
        dataset_name: Name of dataset (e.g., "MNIST-100", "SIFT-100")
        out_path: Output path for comparison plot
    """
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    cmap = get_custom_colormap()
    colors_algo = {'LSH': '#1f77b4', 'Hypercube': '#ff7f0e', 'IVFFlat': '#2ca02c', 'IVFPQ': '#d62728'}
    markers_algo = {'LSH': 'o', 'Hypercube': 's', 'IVFFlat': '^', 'IVFPQ': 'D'}
    
    all_ttrue = []
    
    # Collect data from all algorithms
    algo_data = {}
    for algo_name, csv_path in csv_paths.items():
        if not os.path.exists(csv_path):
            print(f"Warning: {csv_path} not found, skipping {algo_name}")
            continue
            
        headers, rows = read_csv(csv_path)
        recall = np.array([to_float(r.get('AvgRecall', 'nan')) for r in rows])
        qps = np.array([to_float(r.get('QPS', 'nan')) for r in rows])
        tapprox = np.array([to_float(r.get('AvgTApprox', 'nan')) for r in rows])
        ttrue = np.array([to_float(r.get('AvgTTrue', 'nan')) for r in rows])
        speedup = np.where((tapprox > 0) & ~np.isnan(tapprox) & ~np.isnan(ttrue), 
                          ttrue / tapprox, np.nan)
        
        algo_data[algo_name] = {
            'recall': recall,
            'qps': qps,
            'tapprox': tapprox,
            'ttrue': ttrue,
            'speedup': speedup,
            'color': colors_algo.get(algo_name, '#333333'),
            'marker': markers_algo.get(algo_name, 'o')
        }
        all_ttrue.extend(ttrue[~np.isnan(ttrue)])
    
    if not algo_data:
        print("Error: No valid data found")
        return
    
    vmax = max(all_ttrue) if all_ttrue else 1.0
    
    # Plot 1: Recall vs QPS
    ax = axes[0]
    for algo_name, data in algo_data.items():
        # Plot all points
        sc = ax.scatter(data['qps'], data['recall'], 
                       c=data['tapprox'], cmap=cmap, vmin=0, vmax=vmax,
                       marker=data['marker'], s=60, alpha=0.6,
                       edgecolors=data['color'], linewidths=1.5,
                       label=f"{algo_name}")
        
        # Plot Pareto frontier
        pareto_idx = compute_pareto_frontier(data['qps'], data['recall'])
        if len(pareto_idx) > 0:
            p_qps = data['qps'][pareto_idx]
            p_recall = data['recall'][pareto_idx]
            sort_idx = np.argsort(p_qps)
            ax.plot(p_qps[sort_idx], p_recall[sort_idx], 
                   color=data['color'], linestyle='--', linewidth=2, alpha=0.8)
    
    ax.set_xlabel('QPS (Queries Per Second)', fontsize=11)
    ax.set_ylabel('Recall', fontsize=11)
    ax.set_title(f'{dataset_name}: Recall vs QPS', fontsize=12, weight='bold')
    ax.legend(loc='best')
    ax.grid(True, alpha=0.3)
    
    # Plot 2: Recall vs Speedup
    ax = axes[1]
    for algo_name, data in algo_data.items():
        sc = ax.scatter(data['speedup'], data['recall'],
                       c=data['tapprox'], cmap=cmap, vmin=0, vmax=vmax,
                       marker=data['marker'], s=60, alpha=0.6,
                       edgecolors=data['color'], linewidths=1.5,
                       label=f"{algo_name}")
        
        # Plot Pareto frontier
        pareto_idx = compute_pareto_frontier(data['speedup'], data['recall'])
        if len(pareto_idx) > 0:
            p_speedup = data['speedup'][pareto_idx]
            p_recall = data['recall'][pareto_idx]
            sort_idx = np.argsort(p_speedup)
            ax.plot(p_speedup[sort_idx], p_recall[sort_idx],
                   color=data['color'], linestyle='--', linewidth=2, alpha=0.8)
    
    ax.set_xlabel('Speedup Factor (tTrue / tApprox)', fontsize=11)
    ax.set_ylabel('Recall', fontsize=11)
    ax.set_title(f'{dataset_name}: Recall vs Speedup', fontsize=12, weight='bold')
    ax.legend(loc='best')
    ax.grid(True, alpha=0.3)
    
    # Plot 3: Recall vs Time
    ax = axes[2]
    for algo_name, data in algo_data.items():
        sc = ax.scatter(data['tapprox'], data['recall'],
                       c=data['tapprox'], cmap=cmap, vmin=0, vmax=vmax,
                       marker=data['marker'], s=60, alpha=0.6,
                       edgecolors=data['color'], linewidths=1.5,
                       label=f"{algo_name}")
        
        # Plot Pareto frontier (minimize time, maximize recall)
        pareto_idx = compute_pareto_frontier(-data['tapprox'], data['recall'])
        if len(pareto_idx) > 0:
            p_time = data['tapprox'][pareto_idx]
            p_recall = data['recall'][pareto_idx]
            sort_idx = np.argsort(p_time)
            ax.plot(p_time[sort_idx], p_recall[sort_idx],
                   color=data['color'], linestyle='--', linewidth=2, alpha=0.8)
    
    ax.set_xlabel('AvgTApprox (seconds)', fontsize=11)
    ax.set_ylabel('Recall', fontsize=11)
    ax.set_title(f'{dataset_name}: Recall vs Time', fontsize=12, weight='bold')
    ax.legend(loc='best')
    ax.grid(True, alpha=0.3)
    
    plt.suptitle(f'Algorithm Comparison on {dataset_name}', 
                fontsize=14, weight='bold', y=0.98)
    plt.tight_layout(rect=[0, 0.08, 1, 0.96])
    
    # Add colorbar at the bottom with more space
    cbar = fig.colorbar(sc, ax=axes, orientation='horizontal', pad=0.15, aspect=40, shrink=0.8)
    cbar.set_label('AvgTApprox (s)', fontsize=11)
    
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close(fig)
    
    print(f"✓ Saved: {out_path}")


def main():
    print("\nGenerating algorithm comparison plots...\n")
    
    # MNIST comparisons
    mnist_csvs = {
        'LSH': 'runs/lsh_mnist_100_grid.csv',
        'Hypercube': 'runs/hypercube_mnist_custom_grid.csv'
    }
    compare_algorithms(mnist_csvs, 'MNIST-100', 'runs/comparison_mnist.png')
    
    # SIFT comparisons
    sift_csvs = {
        'LSH': 'runs/lsh_sift_100/lsh_sift_100_grid.csv',
        'Hypercube': 'runs/hypercube_sift_100_grid.csv'
    }
    compare_algorithms(sift_csvs, 'SIFT-100', 'runs/comparison_sift.png')
    
    print("\n✓ All comparison plots generated!")
    print("\nFiles created:")
    print("  - runs/comparison_mnist.png")
    print("  - runs/comparison_sift.png")
    print("\nThese plots show how LSH and Hypercube compare on each dataset.")


if __name__ == '__main__':
    main()
