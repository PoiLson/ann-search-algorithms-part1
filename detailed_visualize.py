#!/usr/bin/env python3
"""
Create a detailed LSH visualization without GUI display
Saves plots as PNG files for analysis
"""

import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
import numpy as np
import re
from collections import defaultdict

def parse_results(filename='results.txt'):
    """Parse hash table data from results file"""
    hash_tables = defaultdict(dict)
    current_table = None
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Parse hash table headers
            if line.startswith('Hash table'):
                match = re.search(r'Hash table (\d+):', line)
                if match:
                    current_table = int(match.group(1))
            
            # Parse bucket data
            elif line.startswith('Bucket') and current_table is not None:
                match = re.search(r'Bucket (\d+):', line)
                if match:
                    bucket_id = int(match.group(1))
                    
                    # Extract all (x, y) coordinates
                    points = []
                    coords = re.findall(r'\(([0-9.-]+),\s*([0-9.-]+)\)', line)
                    for x, y in coords:
                        points.append((float(x), float(y)))
                    
                    hash_tables[current_table][bucket_id] = points
    
    return dict(hash_tables)

def create_comprehensive_visualization(hash_tables):
    """Create detailed LSH visualizations"""
    
    # Create a large figure with multiple subplots
    fig = plt.figure(figsize=(20, 12))
    
    # Define colors for buckets
    colors = plt.cm.tab20(np.linspace(0, 1, 20))
    colors = list(colors) + list(plt.cm.Set3(np.linspace(0, 1, 12)))
    
    num_tables = len(hash_tables)
    
    # 1. Individual hash table scatter plots
    for i, (table_id, buckets) in enumerate(hash_tables.items()):
        ax = plt.subplot(2, num_tables, i + 1)
        
        # Plot each bucket with different color
        for bucket_id, points in buckets.items():
            if points:
                xs = [p[0] for p in points]
                ys = [p[1] for p in points]
                
                color = colors[bucket_id % len(colors)]
                ax.scatter(xs, ys, c=[color], label=f'B{bucket_id}', 
                          s=60, alpha=0.8, edgecolors='black', linewidth=0.5)
        
        ax.set_title(f'Hash Table {table_id}\n({len([p for bucket in buckets.values() for p in bucket])} points, {len(buckets)} buckets)', 
                    fontsize=12, fontweight='bold')
        ax.set_xlabel('X Coordinate')
        ax.set_ylabel('Y Coordinate')
        ax.grid(True, alpha=0.3)
        ax.set_xlim(-5, 105)
        ax.set_ylim(-5, 105)
        
        # Add legend in a compact format
        handles, labels = ax.get_legend_handles_labels()
        if len(handles) <= 10:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        else:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=6, ncol=2)
    
    # 2. Bucket size histograms
    for i, (table_id, buckets) in enumerate(hash_tables.items()):
        ax = plt.subplot(2, num_tables, i + 1 + num_tables)
        
        bucket_sizes = [len(points) for points in buckets.values()]
        bucket_ids = list(buckets.keys())
        
        bars = ax.bar(bucket_ids, bucket_sizes, color='lightcoral', 
                     edgecolor='darkred', alpha=0.7, width=0.8)
        
        # Add value labels on bars
        for bar, size in zip(bars, bucket_sizes):
            if size > 0:
                ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1,
                       str(size), ha='center', va='bottom', fontsize=9, fontweight='bold')
        
        ax.set_title(f'Hash Table {table_id} - Bucket Sizes', fontsize=12, fontweight='bold')
        ax.set_xlabel('Bucket ID')
        ax.set_ylabel('Number of Points')
        ax.grid(True, alpha=0.3, axis='y')
        ax.set_ylim(0, max(bucket_sizes) + 1)
    
    plt.tight_layout()
    return fig

def create_distribution_analysis(hash_tables):
    """Create analysis plots for distribution quality"""
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    
    # 1. Bucket size distribution comparison
    ax1 = axes[0, 0]
    for table_id, buckets in hash_tables.items():
        bucket_sizes = [len(points) for points in buckets.values()]
        ax1.hist(bucket_sizes, bins=range(0, max(bucket_sizes)+2), alpha=0.7, 
                label=f'Table {table_id}', edgecolor='black')
    
    ax1.set_title('Bucket Size Distribution', fontweight='bold')
    ax1.set_xlabel('Bucket Size (Number of Points)')
    ax1.set_ylabel('Frequency')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # 2. Load factor comparison
    ax2 = axes[0, 1]
    table_ids = list(hash_tables.keys())
    load_factors = []
    
    for table_id, buckets in hash_tables.items():
        total_points = sum(len(points) for points in buckets.values())
        total_buckets = len(buckets)
        load_factors.append(total_points / total_buckets)
    
    bars = ax2.bar(table_ids, load_factors, color='skyblue', edgecolor='navy')
    for bar, lf in zip(bars, load_factors):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.05,
                f'{lf:.2f}', ha='center', va='bottom', fontweight='bold')
    
    ax2.set_title('Load Factor per Hash Table', fontweight='bold')
    ax2.set_xlabel('Hash Table ID')
    ax2.set_ylabel('Load Factor')
    ax2.grid(True, alpha=0.3)
    
    # 3. Point overlap between tables (Jaccard similarity per bucket)
    if len(hash_tables) >= 2:
        ax3 = axes[1, 0]
        
        # Get the first two tables for comparison
        tables = list(hash_tables.items())[:2]
        (table1_id, table1), (table2_id, table2) = tables
        
        # Compare bucket overlaps
        max_buckets = max(len(table1), len(table2))
        similarities = []
        bucket_pairs = []
        
        for bucket_id in range(max_buckets):
            if bucket_id in table1 and bucket_id in table2:
                set1 = set(table1[bucket_id])
                set2 = set(table2[bucket_id])
                
                intersection = len(set1.intersection(set2))
                union = len(set1.union(set2))
                
                if union > 0:
                    similarity = intersection / union
                    similarities.append(similarity)
                    bucket_pairs.append(bucket_id)
        
        if similarities:
            ax3.bar(bucket_pairs, similarities, color='lightgreen', edgecolor='darkgreen')
            ax3.set_title(f'Bucket Similarity\n(Table {table1_id} vs Table {table2_id})', fontweight='bold')
            ax3.set_xlabel('Bucket ID')
            ax3.set_ylabel('Jaccard Similarity')
            ax3.grid(True, alpha=0.3)
            ax3.set_ylim(0, 1)
        else:
            ax3.text(0.5, 0.5, 'No overlapping buckets', transform=ax3.transAxes, 
                    ha='center', va='center', fontsize=12)
    
    # 4. Standard deviation comparison
    ax4 = axes[1, 1]
    std_devs = []
    
    for table_id, buckets in hash_tables.items():
        bucket_sizes = [len(points) for points in buckets.values()]
        std_devs.append(np.std(bucket_sizes))
    
    bars = ax4.bar(table_ids, std_devs, color='orange', edgecolor='darkorange')
    for bar, std in zip(bars, std_devs):
        ax4.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.02,
                f'{std:.2f}', ha='center', va='bottom', fontweight='bold')
    
    ax4.set_title('Bucket Size Standard Deviation', fontweight='bold')
    ax4.set_xlabel('Hash Table ID')
    ax4.set_ylabel('Standard Deviation')
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def print_detailed_analysis(hash_tables):
    """Print comprehensive analysis"""
    print("\n" + "="*80)
    print("DETAILED LSH HASH TABLE ANALYSIS")
    print("="*80)
    
    # Overall statistics
    total_points = sum(len([p for bucket in buckets.values() for p in bucket]) 
                      for buckets in hash_tables.values())
    print(f"Total points across all tables: {total_points}")
    print(f"Number of hash tables: {len(hash_tables)}")
    
    # Per-table analysis
    for table_id, buckets in hash_tables.items():
        print(f"\n{'='*40}")
        print(f"HASH TABLE {table_id}")
        print('='*40)
        
        # Basic stats
        table_points = sum(len(points) for points in buckets.values())
        non_empty = sum(1 for points in buckets.values() if points)
        total_buckets = len(buckets)
        
        print(f"Points: {table_points}")
        print(f"Buckets (total): {total_buckets}")
        print(f"Buckets (non-empty): {non_empty}")
        print(f"Buckets (empty): {total_buckets - non_empty}")
        print(f"Load factor: {table_points/total_buckets:.3f}")
        
        # Distribution stats
        sizes = [len(points) for points in buckets.values() if points]
        if sizes:
            print(f"Avg bucket size: {np.mean(sizes):.2f} ± {np.std(sizes):.2f}")
            print(f"Bucket size range: {min(sizes)} - {max(sizes)}")
            print(f"Median bucket size: {np.median(sizes):.1f}")
        
        # Show largest and smallest buckets
        bucket_info = [(bid, len(points)) for bid, points in buckets.items()]
        bucket_info.sort(key=lambda x: x[1], reverse=True)
        
        print(f"\nLargest buckets:")
        for bid, size in bucket_info[:5]:
            print(f"  Bucket {bid:2d}: {size:2d} points")
        
        print(f"\nSmallest non-empty buckets:")
        non_empty_buckets = [(bid, size) for bid, size in bucket_info if size > 0]
        for bid, size in non_empty_buckets[-5:]:
            print(f"  Bucket {bid:2d}: {size:2d} points")

def main():
    """Main execution function"""
    try:
        print("Parsing LSH hash table results...")
        hash_tables = parse_results('results.txt')
        
        if not hash_tables:
            print("No hash table data found in results.txt")
            return
        
        print(f"Successfully parsed {len(hash_tables)} hash table(s)")
        
        # Detailed analysis
        print_detailed_analysis(hash_tables)
        
        # Create visualizations
        print("\nCreating comprehensive visualization...")
        fig1 = create_comprehensive_visualization(hash_tables)
        fig1.savefig('lsh_comprehensive.png', dpi=300, bbox_inches='tight')
        print("Saved: lsh_comprehensive.png")
        
        print("Creating distribution analysis...")
        fig2 = create_distribution_analysis(hash_tables)
        fig2.savefig('lsh_analysis.png', dpi=300, bbox_inches='tight')
        print("Saved: lsh_analysis.png")
        
        plt.close('all')  # Clean up memory
        
        print("\n" + "="*80)
        print("VISUALIZATION COMPLETE!")
        print("="*80)
        print("Generated files:")
        print("  - lsh_comprehensive.png : Individual table visualizations")
        print("  - lsh_analysis.png     : Distribution quality analysis")
        print("  - lsh_hash_tables.png  : Simple overview (from previous script)")
        
    except FileNotFoundError:
        print("Error: results.txt not found!")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()