#!/usr/bin/env python3
"""
Visualize MNIST images from IDX3 binary format.
Usage: python view_mnist.py <idx3_file> <image_index>
"""

import sys
import struct
import numpy as np
import matplotlib.pyplot as plt

def read_be_u32(f):
    """Read a big-endian 32-bit unsigned integer."""
    return struct.unpack('>I', f.read(4))[0]

def read_mnist_idx3(filepath, image_index=0):
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
        
        print(f"File: {filepath}")
        print(f"Magic: {magic}")
        print(f"Images: {num_images}")
        print(f"Size: {rows}x{cols}")
        
        if image_index >= num_images:
            raise ValueError(f"Image index {image_index} out of range (0-{num_images-1})")
        
        # Skip to the desired image
        dimension = rows * cols
        f.seek(16 + image_index * dimension)  # 16-byte header + offset
        
        # Read the image
        pixels = f.read(dimension)
        if len(pixels) != dimension:
            raise ValueError(f"Could not read complete image {image_index}")
        
        # Convert to numpy array and reshape
        img = np.frombuffer(pixels, dtype=np.uint8).reshape(rows, cols)
        
        return img

def visualize_mnist(img, title="MNIST Image", invert=True):
    """
    Visualize a MNIST image.
    
    Args:
        img: numpy array of shape (28, 28)
        title: Title for the plot
        invert: If True, use white background (0=white, 255=black)
    """
    plt.figure(figsize=(6, 6))
    
    if invert:
        # Use inverted grayscale: 0=white, 255=black
        plt.imshow(img, cmap='gray_r', vmin=0, vmax=255)
    else:
        # Standard grayscale: 0=black, 255=white
        plt.imshow(img, cmap='gray', vmin=0, vmax=255)
    
    plt.title(title)
    plt.colorbar(label='Pixel Value')
    plt.axis('off')
    
    # Print pixel statistics
    print(f"\nPixel Statistics:")
    print(f"  Min: {img.min()}")
    print(f"  Max: {img.max()}")
    print(f"  Mean: {img.mean():.2f}")
    print(f"  Std: {img.std():.2f}")
    
    # Show a small region for debugging
    print(f"\nTop-left 5x5 corner:")
    print(img[:5, :5])
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python view_mnist.py <idx3_file> [image_index]")
        print("Example: python view_mnist.py Mnist_data/train-images.idx3-ubyte 0")
        sys.exit(1)
    
    filepath = sys.argv[1]
    image_index = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    
    try:
        img = read_mnist_idx3(filepath, image_index)
        visualize_mnist(img, title=f"MNIST Image {image_index}", invert=True)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
