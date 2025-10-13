#!/usr/bin/env python3
"""
LSH Hash Table Visualization Script
Visualizes the hash table buckets and point distributions from results.txt
"""

import matplotlib.pyplot as plt
import numpy as np
import re
from collections import defaultdict
import seaborn as sns

def parse_hash_tables(filename):
    """
    Parse the hash table data from results.txt
    Returns a dictionary of hash tables with their buckets and points
    """
    hash_tables = {}
    current_table = None
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    for line in lines:
        line = line.strip()
        
        # Check for hash table header
        if line.startswith('Hash table'):
            table_match = re.match(r'Hash table (\d+):', line)
            if table_match:
                current_table = int(table_match.group(1))
                hash_tables[current_table] = {}
        
        # Check for bucket data
        elif line.startswith('Bucket'):
            if current_table is not None:
                bucket_match = re.match(r'Bucket (\d+):', line)
                if bucket_match:
                    bucket_num = int(bucket_match.group(1))
                    
                    # Extract points from the line
                    points = []
                    point_pattern = r'\(([0-9.-]+),\s*([0-9.-]+)\)'
                    matches = re.findall(point_pattern, line)
                    
                    for match in matches:
                        x, y = float(match[0]), float(match[1])
                        points.append((x, y))
                    
                    hash_tables[current_table][bucket_num] = points
    
    return hash_tables

def visualize_hash_tables(hash_tables):
    """
    Create comprehensive visualizations of the hash table data
    """
    # Set up the plotting style
    plt.style.use('default')
    sns.set_palette("husl")
    
    num_tables = len(hash_tables)
    
    # Create figure with subplots
    fig = plt.figure(figsize=(20, 5 * num_tables))
    
    # Colors for different buckets
    colors = plt.cm.tab20(np.linspace(0, 1, 20))  # Up to 20 different colors
    
    for table_idx, (table_id, table_data) in enumerate(hash_tables.items()):
        # Create subplot for this hash table
        ax = plt.subplot(num_tables, 3, table_idx * 3 + 1)
        
        # Plot points by bucket
        all_points_x, all_points_y = [], []
        bucket_stats = []
        
        for bucket_id, points in table_data.items():
            if points:  # Only plot non-empty buckets
                x_coords = [p[0] for p in points]
                y_coords = [p[1] for p in points]
                
                # Use color based on bucket ID
                color = colors[bucket_id % len(colors)]
                ax.scatter(x_coords, y_coords, c=[color], label=f'Bucket {bucket_id}', 
                          s=50, alpha=0.7, edgecolors='black', linewidth=0.5)
                
                all_points_x.extend(x_coords)
                all_points_y.extend(y_coords)
                bucket_stats.append(len(points))
        
        ax.set_title(f'Hash Table {table_id} - Point Distribution by Bucket')
        ax.set_xlabel('X Coordinate')
        ax.set_ylabel('Y Coordinate')
        ax.grid(True, alpha=0.3)
        ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
        
        # Bucket size histogram
        ax2 = plt.subplot(num_tables, 3, table_idx * 3 + 2)
        if bucket_stats:
            ax2.bar(range(len(bucket_stats)), bucket_stats, color='skyblue', alpha=0.7)
            ax2.set_title(f'Hash Table {table_id} - Bucket Sizes')
            ax2.set_xlabel('Bucket Index')
            ax2.set_ylabel('Number of Points')
            ax2.grid(True, alpha=0.3)
        
        # Overall point distribution
        ax3 = plt.subplot(num_tables, 3, table_idx * 3 + 3)
        if all_points_x and all_points_y:
            ax3.scatter(all_points_x, all_points_y, c='red', alpha=0.6, s=30)
            ax3.set_title(f'Hash Table {table_id} - All Points')
            ax3.set_xlabel('X Coordinate')
            ax3.set_ylabel('Y Coordinate')
            ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('hash_table_visualization.png', dpi=300, bbox_inches='tight')
    plt.show()

def analyze_distribution(hash_tables):
    """
    Analyze and print statistics about the hash table distribution
    """
    print("=== LSH Hash Table Analysis ===\n")
    
    for table_id, table_data in hash_tables.items():
        print(f"Hash Table {table_id}:")
        
        total_points = sum(len(points) for points in table_data.values())
        non_empty_buckets = sum(1 for points in table_data.values() if points)
        total_buckets = len(table_data)
        
        bucket_sizes = [len(points) for points in table_data.values() if points]
        
        print(f"  Total points: {total_points}")
        print(f"  Total buckets: {total_buckets}")
        print(f"  Non-empty buckets: {non_empty_buckets}")
        print(f"  Load factor: {total_points/total_buckets:.2f}")
        
        if bucket_sizes:
            print(f"  Average bucket size: {np.mean(bucket_sizes):.2f}")
            print(f"  Max bucket size: {max(bucket_sizes)}")
            print(f"  Min bucket size: {min(bucket_sizes)}")
            print(f"  Std deviation: {np.std(bucket_sizes):.2f}")
        
        print()

def create_bucket_comparison(hash_tables):
    """
    Create a comparison visualization showing how the same points are distributed 
    across different hash tables
    """
    plt.figure(figsize=(15, 10))
    
    # Collect all unique points from all tables
    all_points = set()
    for table_data in hash_tables.values():
        for points in table_data.values():
            for point in points:
                all_points.add(point)
    
    all_points = list(all_points)
    
    # Create a mapping of points to their bucket assignments in each table
    point_to_buckets = {}
    for point in all_points:
        point_to_buckets[point] = {}
        
        for table_id, table_data in hash_tables.items():
            for bucket_id, bucket_points in table_data.items():
                if point in bucket_points:
                    point_to_buckets[point][table_id] = bucket_id
    
    # Plot points colored by their bucket in the first hash table
    if 0 in hash_tables:  # If hash table 0 exists
        colors = plt.cm.tab10(np.linspace(0, 1, 10))
        
        for bucket_id, points in hash_tables[0].items():
            if points:
                x_coords = [p[0] for p in points]
                y_coords = [p[1] for p in points]
                color = colors[bucket_id % len(colors)]
                
                plt.scatter(x_coords, y_coords, c=[color], label=f'Table 0 - Bucket {bucket_id}', 
                           s=60, alpha=0.7, edgecolors='black', linewidth=0.5)
    
    plt.title('Point Distribution - Colored by Hash Table 0 Buckets')
    plt.xlabel('X Coordinate')
    plt.ylabel('Y Coordinate')
    plt.grid(True, alpha=0.3)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    plt.savefig('bucket_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    """
    Main function to run the visualization
    """
    try:
        # Parse the hash table data
        print("Parsing hash table data from results.txt...")
        hash_tables = parse_hash_tables('results.txt')
        
        if not hash_tables:
            print("No hash table data found in results.txt")
            return
        
        print(f"Found {len(hash_tables)} hash tables")
        
        # Analyze distribution
        analyze_distribution(hash_tables)
        
        # Create visualizations
        print("Creating visualizations...")
        visualize_hash_tables(hash_tables)
        create_bucket_comparison(hash_tables)
        
        print("Visualizations saved as:")
        print("  - hash_table_visualization.png")
        print("  - bucket_comparison.png")
        
    except FileNotFoundError:
        print("Error: results.txt file not found!")
        print("Make sure to run the LSH program first to generate results.txt")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()