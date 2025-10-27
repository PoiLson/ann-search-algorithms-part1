#ifndef IVFFLAT_H
#define IVFFLAT_H

int findSubsetSize(int subsetSize);

void fisher_yates_shuffle(void **array, size_t n);
Dataset* createSubset(Dataset* dataset, int subsetSize);

void ivfflat_init(Dataset* dataset, int kclusters);
void runKmeans(Dataset* dataset, int kclusters);
void lloydAlgorithm(void** subset);



#endif