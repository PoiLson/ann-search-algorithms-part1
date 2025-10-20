#ifndef DATASETS_H
#define DATASETS_H

// Definition of Dataset struct
// Holds dataset information: data points, size, and dimension
typedef struct Dataset
{
    void** data;
    int size;
    int dimension;
} Dataset;

// reads dataset from a file(dataset_path) and returns a pointer to a Dataset struct
Dataset* read_data(const char* dataset_path);

// reads MNIST IDX3 images file (big-endian) and returns a Dataset of floats [0..255]
Dataset* read_data_mnist(const char* images_path);

//helper: prints first 'size' points of the dataset. used for debugging
void printPartialDataset(int size, const Dataset* dataset);

// frees all memory associated with a Dataset allocated by read_data
void free_dataset(Dataset* dataset);

#endif