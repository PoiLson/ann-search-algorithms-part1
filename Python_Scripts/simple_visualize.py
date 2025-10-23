#!/usr/bin/env python3
"""
Simple LSH Hash Table Visualization
A simplified version focusing on the most important visualizations
"""

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

def plot_hash_table_distribution(hash_tables):
    """Create a comprehensive visualization of the hash table distribution"""
    
    num_tables = len(hash_tables)
    fig, axes = plt.subplots(2, num_tables, figsize=(6*num_tables, 10))
    
    if num_tables == 1:
        axes = axes.reshape(2, 1)
    
    # Colors for different buckets 
    colors = plt.cm.Set3(np.linspace(0, 1, 25))
    
    for i, (table_id, buckets) in enumerate(hash_tables.items()):
        
        # Top row: Scatter plot of points colored by bucket
        ax1 = axes[0, i]
        
        for bucket_id, points in buckets.items():
            if points:  # Only plot non-empty buckets
                xs = [p[0] for p in points]
                ys = [p[1] for p in points]
                
                color = colors[bucket_id % len(colors)]
                ax1.scatter(xs, ys, c=[color], label=f'B{bucket_id}', 
                           s=50, alpha=0.8, edgecolors='black', linewidth=0.3)
        
        ax1.set_title(f'Hash Table {table_id} - Points by Bucket')
        ax1.set_xlabel('X Coordinate')
        ax1.set_ylabel('Y Coordinate')
        ax1.grid(True, alpha=0.3)
        ax1.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        
        # Bottom row: Bucket size distribution
        ax2 = axes[1, i]
        
        bucket_sizes = [len(points) for points in buckets.values()]
        bucket_ids = list(buckets.keys())
        
        bars = ax2.bar(bucket_ids, bucket_sizes, color='lightblue', 
                      edgecolor='navy', alpha=0.7)
        
        # Add value labels on bars
        for bar, size in zip(bars, bucket_sizes):
            if size > 0:
                ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1,
                        str(size), ha='center', va='bottom', fontsize=8)
        
        ax2.set_title(f'Hash Table {table_id} - Bucket Sizes')
        ax2.set_xlabel('Bucket ID')
        ax2.set_ylabel('Number of Points')
        ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def print_statistics(hash_tables):
    """Print detailed statistics about the hash tables"""
    print("\n" + "="*60)
    print("LSH HASH TABLE ANALYSIS")
    print("="*60)
    
    for table_id, buckets in hash_tables.items():
        print(f"\nHash Table {table_id}:")
        print("-" * 30)
        
        # Basic statistics
        total_points = sum(len(points) for points in buckets.values())
        non_empty_buckets = sum(1 for points in buckets.values() if points)
        total_buckets = len(buckets)
        
        print(f"Total points: {total_points}")
        print(f"Total buckets: {total_buckets}")
        print(f"Non-empty buckets: {non_empty_buckets}")
        print(f"Empty buckets: {total_buckets - non_empty_buckets}")
        print(f"Load factor: {total_points/total_buckets:.2f}")
        
        # Bucket size statistics
        sizes = [len(points) for points in buckets.values() if points]
        if sizes:
            print(f"Average bucket size: {np.mean(sizes):.2f}")
            print(f"Largest bucket: {max(sizes)} points")
            print(f"Smallest non-empty bucket: {min(sizes)} points")
            print(f"Standard deviation: {np.std(sizes):.2f}")
        
        # Show bucket contents summary
        print("\nBucket distribution:")
        for bucket_id, points in sorted(buckets.items()):
            if points:
                print(f"  Bucket {bucket_id:2d}: {len(points):2d} points")
    
    print("\n" + "="*60)

def main():
    """Main execution function"""
    try:
        print("Parsing LSH hash table results...")
        
        # Parse the data
        hash_tables = parse_results('results.txt')
        
        if not hash_tables:
            print("No hash table data found in results.txt")
            return
        
        print(f"Successfully parsed {len(hash_tables)} hash table(s)")
        
        # Print statistics
        print_statistics(hash_tables)
        
        # Create visualization
        print("\nCreating visualization...")
        fig = plot_hash_table_distribution(hash_tables)
        
        # Save the plot
        output_file = 'lsh_hash_tables.png'
        fig.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Visualization saved as: {output_file}")
        
        # Show the plot
        plt.show()
        
    except FileNotFoundError:
        print("Error: results.txt not found!")
        print("Please make sure to run your LSH program first to generate results.txt")
    except Exception as e:
        print(f"Error occurred: {e}")

if __name__ == "__main__":
    main()