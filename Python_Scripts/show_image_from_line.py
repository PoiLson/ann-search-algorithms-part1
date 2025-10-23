#!/usr/bin/env python3
"""
Visualize a 28x28 MNIST image from a line of 784 pixel values.
Usage: python show_image_from_line.py "pixel1 pixel2 pixel3 ..."
       or paste the line when prompted
"""

import sys
import numpy as np
import matplotlib.pyplot as plt

def parse_pixel_line(line):
    """
    Parse a line of pixel values (space or comma separated).
    
    Args:
        line: String containing 784 pixel values
    
    Returns:
        numpy array of shape (28, 28)
    """
    # Handle both space and comma separation
    line = line.replace(',', ' ')
    values = line.strip().split()
    
    # Convert to integers or floats
    try:
        pixels = [float(v) for v in values]
    except ValueError:
        raise ValueError("Could not parse all values as numbers")
    
    if len(pixels) != 784:
        raise ValueError(f"Expected 784 values (28x28), got {len(pixels)}")
    
    # Reshape to 28x28
    img = np.array(pixels).reshape(28, 28)
    
    return img

def visualize_image(img, title="MNIST Image", invert=True):
    """
    Visualize a MNIST image.
    
    Args:
        img: numpy array of shape (28, 28)
        title: Title for the plot
        invert: If True, use white background (0=white, 255=black)
    """
    plt.figure(figsize=(8, 8))
    
    if invert:
        # Use inverted grayscale: 0=white, 255=black
        plt.imshow(img, cmap='gray_r', vmin=0, vmax=255)
    else:
        # Standard grayscale: 0=black, 255=white
        plt.imshow(img, cmap='gray', vmin=0, vmax=255)
    
    plt.title(title, fontsize=14)
    plt.colorbar(label='Pixel Value', shrink=0.8)
    plt.axis('off')
    
    # Print pixel statistics
    print(f"\nPixel Statistics:")
    print(f"  Min: {img.min():.2f}")
    print(f"  Max: {img.max():.2f}")
    print(f"  Mean: {img.mean():.2f}")
    print(f"  Std: {img.std():.2f}")
    print(f"  Non-zero pixels: {np.count_nonzero(img)}")
    
    # Show a small region for debugging
    print(f"\nTop-left 5x5 corner:")
    print(img[:5, :5].astype(int))
    
    print(f"\nCenter 5x5 region:")
    print(img[12:17, 12:17].astype(int))
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # Line provided as argument
        pixel_line = ' '.join(sys.argv[1:])
    else:
        # Prompt for input
        print("Paste a line of 784 pixel values (space or comma separated):")
        pixel_line = input().strip()
    
    try:
        img = parse_pixel_line(pixel_line)
        visualize_image(img, title="MNIST Image from Line", invert=True)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
