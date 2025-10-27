#ifndef IVFFLAT_H
#define IVFFLAT_H

void fisher_yates_shuffle(void *array, size_t n, size_t elem_size);
int findSubsetSize(int subsetSize);
void** createSubset(Dataset* dataset, int subsetSize);
void lloydAlgorithm(void** subset);



#endif