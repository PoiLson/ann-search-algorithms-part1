#include "../include/main.h"

// Generic Fisher–Yates shuffle
// Generic Fisher–Yates shuffle for arrays of pointers
void fisher_yates_shuffle(void **array, size_t n) {
    srand((unsigned int) time(NULL));

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        // Swap the pointers
        void *tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}


int findSubsetSize(int subsetSize)
{
    int size = sqrt(subsetSize);
    return size;
}

void** createSubset(Dataset* dataset, int subsetSize)
{
    printf("hi\n");

    // we randomly sort the given dataset
    // fisher_yates_shuffle(dataset->data)
    void** subset;

    return subset;
}

void lloydAlgorithm(void** subset)
{
    //choose a random element to start as a centroid
}
