#!/usr/bin/env python3
"""
Visualize MNIST query images and their nearest neighbors from output.txt
Usage: python visualize_query_neighbors.py [num_queries] [output_file]
"""

import sys
import struct
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

def read_be_u32(f):
    """Read a big-endian 32-bit unsigned integer."""
    return struct.unpack('>I', f.read(4))[0]

def read_mnist_image(filepath, image_index):
    """
    Read a specific image from MNIST IDX3 file.
    
    Args:
        filepath: Path to the .idx3-ubyte file
        image_index: Which image to read (0-based)
    
    Returns:
        numpy array of shape (28, 28) with pixel values 0-255
    """
    with open(filepath, 'rb') as f:
        magic = read_be_u32(f)
        if magic != 2051:
            raise ValueError(f"Invalid magic number {magic}, expected 2051")
        
        num_images = read_be_u32(f)
        rows = read_be_u32(f)
        cols = read_be_u32(f)
        
        if image_index >= num_images:
            raise ValueError(f"Image index {image_index} out of range (0-{num_images-1})")
        
        # Skip to the desired image
        dimension = rows * cols
        f.seek(16 + image_index * dimension)
        
        # Read the image
        pixels = f.read(dimension)
        if len(pixels) != dimension:
            raise ValueError(f"Could not read complete image {image_index}")
        
        # Convert to numpy array and reshape
        img = np.frombuffer(pixels, dtype=np.uint8).reshape(rows, cols)
        
        return img

def read_mnist_label(filepath, label_index):
    """Read a specific label from MNIST IDX1 file."""
    with open(filepath, 'rb') as f:
        magic = read_be_u32(f)
        if magic != 2049:
            raise ValueError(f"Invalid magic number {magic}, expected 2049")
        
        num_labels = read_be_u32(f)
        
        if label_index >= num_labels:
            raise ValueError(f"Label index {label_index} out of range")
        
        f.seek(8 + label_index)
        label = f.read(1)[0]
        
        return label

def parse_output_file(filepath):
    """
    Parse output.txt to extract query indices and their nearest neighbors.
    
    Returns:
        List of dicts: [{'query_id': 0, 'neighbors': [45374, 18391, 42646], 
                         'distances_approx': [...], 'distances_true': [...]}, ...]
    """
    results = []
    current_query = None
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            
            if line.startswith('Query:'):
                # Start a new query
                query_id = int(line.split(':')[1].strip())
                current_query = {
                    'query_id': query_id,
                    'neighbors': [],
                    'distances_approx': [],
                    'distances_true': []
                }
                results.append(current_query)
            
            elif line.startswith('Nearest neighbor-') and current_query is not None:
                # Extract neighbor index
                neighbor_id = int(line.split(':')[1].strip())
                current_query['neighbors'].append(neighbor_id)
            
            elif line.startswith('distanceApproximate:') and current_query is not None:
                dist = float(line.split(':')[1].strip())
                current_query['distances_approx'].append(dist)
            
            elif line.startswith('distanceTrue:') and current_query is not None:
                dist = float(line.split(':')[1].strip())
                current_query['distances_true'].append(dist)
    
    return results

def visualize_query_and_neighbors(query_id, query_img, query_label, 
                                   neighbor_imgs, neighbor_labels, neighbor_indices,
                                   distances_approx, distances_true):
    """
    Visualize a query image and its nearest neighbors in a grid.
    """
    n_neighbors = len(neighbor_imgs)
    
    # Create figure with grid layout
    fig = plt.figure(figsize=(15, 4))
    gs = GridSpec(1, n_neighbors + 2, figure=fig, wspace=0.3)
    
    # Plot query image
    ax_query = fig.add_subplot(gs[0, 0])
    ax_query.imshow(query_img, cmap='gray_r', vmin=0, vmax=255)
    ax_query.set_title(f'Query {query_id}\nLabel: {query_label}', 
                       fontsize=12, fontweight='bold', color='blue')
    ax_query.axis('off')
    
    # Add arrow
    ax_arrow = fig.add_subplot(gs[0, 1])
    ax_arrow.text(0.5, 0.5, '→', fontsize=40, ha='center', va='center')
    ax_arrow.axis('off')
    
    # Plot neighbor images
    for i, (img, label, idx, dist_approx, dist_true) in enumerate(
        zip(neighbor_imgs, neighbor_labels, neighbor_indices, distances_approx, distances_true)):
        ax = fig.add_subplot(gs[0, i + 2])
        ax.imshow(img, cmap='gray_r', vmin=0, vmax=255)
        ax.set_title(f'NN-{i+1}: {idx}\nLabel: {label}\n' + 
                     f'dApprox: {dist_approx:.1f}\ndTrue: {dist_true:.1f}',
                     fontsize=9)
        ax.axis('off')
        
        # Highlight if correct label
        if label == query_label:
            for spine in ax.spines.values():
                spine.set_edgecolor('green')
                spine.set_linewidth(3)
                spine.set_visible(True)
    
    plt.suptitle(f'Query {query_id} and its Nearest Neighbors', fontsize=14, fontweight='bold')
    plt.tight_layout()
    
    return fig

def main():
    # Parse arguments
    num_queries = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'output.txt'
    
    # File paths
    query_images_file = 'Mnist_data/t10k-images-100-sample.idx3-ubyte'
    query_labels_file = 'Mnist_data/t10k-labels.idx1-ubyte'
    train_images_file = 'Mnist_data/train-images.idx3-ubyte'
    train_labels_file = 'Mnist_data/train-labels.idx1-ubyte'
    
    print(f"Parsing {output_file}...")
    results = parse_output_file(output_file)
    
    print(f"Found {len(results)} queries in output file")
    print(f"Visualizing first {num_queries} queries...\n")
    
    # Process each query
    for i, result in enumerate(results[:num_queries]):
        query_id = result['query_id']
        neighbor_indices = result['neighbors']
        distances_approx = result['distances_approx']
        distances_true = result['distances_true']
        
        print(f"Processing Query {query_id}...")
        print(f"  Neighbors: {neighbor_indices}")
        
        try:
            # Load query image and label
            query_img = read_mnist_image(query_images_file, query_id)
            query_label = read_mnist_label(query_labels_file, query_id)
            
            # Load neighbor images and labels
            neighbor_imgs = []
            neighbor_labels = []
            for neighbor_idx in neighbor_indices:
                neighbor_img = read_mnist_image(train_images_file, neighbor_idx)
                neighbor_label = read_mnist_label(train_labels_file, neighbor_idx)
                neighbor_imgs.append(neighbor_img)
                neighbor_labels.append(neighbor_label)
            
            # Visualize
            fig = visualize_query_and_neighbors(
                query_id, query_img, query_label,
                neighbor_imgs, neighbor_labels, neighbor_indices,
                distances_approx, distances_true
            )
            
            # Save or show
            output_filename = f'query_{query_id}_neighbors.png'
            fig.savefig(output_filename, dpi=150, bbox_inches='tight')
            print(f"  Saved to {output_filename}")
            
            plt.close(fig)
            
        except Exception as e:
            print(f"  Error processing query {query_id}: {e}")
            continue
    
    print(f"\n✓ Generated visualizations for {min(num_queries, len(results))} queries")
    print("Files saved as: query_<N>_neighbors.png")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help']:
        print(__doc__)
        print("\nArguments:")
        print("  num_queries  : Number of queries to visualize (default: 5)")
        print("  output_file  : Path to output.txt (default: 'output.txt')")
        print("\nExamples:")
        print("  python visualize_query_neighbors.py")
        print("  python visualize_query_neighbors.py 10")
        print("  python visualize_query_neighbors.py 10 output.txt")
        sys.exit(0)
    
    main()
