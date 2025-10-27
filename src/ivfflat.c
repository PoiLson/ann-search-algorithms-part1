#include "../include/main.h"

// Generic Fisher–Yates shuffle
void fisher_yates_shuffle(void *array, size_t n)
{
    unsigned char *arr = (unsigned char *)array;

    srand((unsigned int) time(NULL));

    for (size_t i = n - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);

        // Swap arr[i] and arr[j]
        float tmp = arr[i]; // tmp is a single float
        arr[i] = arr[j];
        arr[j] = tmp;
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
