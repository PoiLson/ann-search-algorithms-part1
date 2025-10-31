#!/usr/bin/env python3
"""
Create a subset of MNIST train dataset (5000 images)
"""
import struct
import sys

def read_mnist_images(filename):
    """Read MNIST image file and return header + images"""
    with open(filename, 'rb') as f:
        # Read header
        magic = struct.unpack('>I', f.read(4))[0]
        num_images = struct.unpack('>I', f.read(4))[0]
        rows = struct.unpack('>I', f.read(4))[0]
        cols = struct.unpack('>I', f.read(4))[0]
        
        print(f"Original file: magic={magic}, images={num_images}, rows={rows}, cols={cols}")
        
        # Read all image data
        image_size = rows * cols
        images = []
        for _ in range(num_images):
            img = f.read(image_size)
            images.append(img)
        
        return (magic, num_images, rows, cols), images

def write_mnist_subset(filename, header, images, subset_size):
    """Write subset of MNIST images to new file"""
    magic, _, rows, cols = header
    
    with open(filename, 'wb') as f:
        # Write header with new count
        f.write(struct.pack('>I', magic))
        f.write(struct.pack('>I', subset_size))
        f.write(struct.pack('>I', rows))
        f.write(struct.pack('>I', cols))
        
        # Write subset of images
        for i in range(min(subset_size, len(images))):
            f.write(images[i])
    
    print(f"Written {min(subset_size, len(images))} images to {filename}")

def main():
    input_file = 'Data/MNIST/train-images.idx3-ubyte'
    output_file = 'Data/MNIST/train-images-5000.idx3-ubyte'
    subset_size = 5000
    
    print(f"Reading {input_file}...")
    header, images = read_mnist_images(input_file)
    
    print(f"Creating subset of {subset_size} images...")
    write_mnist_subset(output_file, header, images, subset_size)
    
    print("Done!")

if __name__ == '__main__':
    main()
