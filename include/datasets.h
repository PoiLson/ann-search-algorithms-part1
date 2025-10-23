#ifndef DATASETS_H
#define DATASETS_H

// Data type enum to distinguish between int and float datasets
typedef enum {
    DATA_TYPE_INT,    // MNIST: integer values 0-255
    DATA_TYPE_FLOAT   // SIFT: float32 values
} DataType;

// Definition of Dataset struct
// Holds dataset information: data points, size, and dimension
typedef struct Dataset
{
    void** data;
    int size;
    int dimension;
    DataType data_type;  // type of data stored (int or float)
} Dataset;

// reads MNIST IDX3 images file (big-endian) and returns a Dataset of integers [0..255]
Dataset* read_data_mnist(const char* images_path);

// reads SIFT IDX3 image descriptor file (little-endian) and returns a Dataset of floats
Dataset* read_data_sift(const char* images_path);

// reads dataset from a file(dataset_path) and returns a pointer to a Dataset struct
Dataset* read_data_experiment(const char* dataset_path);

//helper: prints first 'size' points of the dataset. used for debugging
void printPartialDataset(int size, const Dataset* dataset);

// frees all memory associated with a Dataset allocated by read_data
void free_dataset(Dataset* dataset);

#endif