#!/usr/bin/env python3
"""
Read MNIST labels from IDX1 binary format.
Usage: python read_mnist_labels.py <idx1_file> [start_index] [count]
"""

import sys
import struct

def read_be_u32(f):
    """Read a big-endian 32-bit unsigned integer."""
    return struct.unpack('>I', f.read(4))[0]

def read_mnist_labels(filepath, start_index=0, count=None):
    """
    Read MNIST labels from IDX1 file.
    
    Args:
        filepath: Path to the .idx1-ubyte file
        start_index: First label to read (0-based)
        count: Number of labels to read (None = all remaining)
    
    Returns:
        List of labels (integers 0-9)
    """
    with open(filepath, 'rb') as f:
        magic = read_be_u32(f)
        if magic != 2049:
            raise ValueError(f"Invalid magic number {magic}, expected 2049 for IDX1")
        
        num_labels = read_be_u32(f)
        
        print(f"File: {filepath}")
        print(f"Magic: {magic}")
        print(f"Total labels: {num_labels}")
        
        if start_index >= num_labels:
            raise ValueError(f"Start index {start_index} out of range (0-{num_labels-1})")
        
        # Skip to start position
        f.seek(8 + start_index)  # 8-byte header + offset
        
        # Determine how many to read
        if count is None:
            count = num_labels - start_index
        else:
            count = min(count, num_labels - start_index)
        
        # Read labels
        label_bytes = f.read(count)
        if len(label_bytes) != count:
            raise ValueError(f"Could not read {count} labels")
        
        labels = list(label_bytes)
        
        return labels, num_labels

def print_labels(labels, start_index=0, per_line=20):
    """Print labels in a formatted grid."""
    print(f"\nLabels (showing {len(labels)} starting from index {start_index}):")
    print("-" * 80)
    
    for i in range(0, len(labels), per_line):
        indices = range(start_index + i, start_index + min(i + per_line, len(labels)))
        chunk = labels[i:i + per_line]
        
        # Print indices
        idx_str = "  ".join(f"{idx:5d}" for idx in indices)
        print(f"Idx:   {idx_str}")
        
        # Print labels
        lbl_str = "  ".join(f"  {lbl:d}  " for lbl in chunk)
        print(f"Label: {lbl_str}")
        print()
    
    # Print label distribution
    print("-" * 80)
    print("Label distribution:")
    for digit in range(10):
        count = labels.count(digit)
        percentage = 100 * count / len(labels) if labels else 0
        bar = '█' * int(percentage / 2)
        print(f"  {digit}: {count:5d} ({percentage:5.2f}%) {bar}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python read_mnist_labels.py <idx1_file> [start_index] [count]")
        print("\nExamples:")
        print("  python read_mnist_labels.py Mnist_data/train-labels.idx1-ubyte")
        print("  python read_mnist_labels.py Mnist_data/t10k-labels.idx1-ubyte 0 100")
        print("  python read_mnist_labels.py Mnist_data/train-labels.idx1-ubyte 1000 50")
        sys.exit(1)
    
    filepath = sys.argv[1]
    start_index = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    count = int(sys.argv[3]) if len(sys.argv) > 3 else None
    
    try:
        labels, total = read_mnist_labels(filepath, start_index, count)
        print_labels(labels, start_index)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
