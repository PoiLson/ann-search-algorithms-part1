#ifndef DATASETS_H
#define DATASETS_H

typedef struct Dataset
{
    void** data;
    int size;
    int dimension;
} Dataset;


Dataset* read_data(const char* dataset_path);
void printPartialDataset(int size, const Dataset* dataset);

#endif