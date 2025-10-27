#include "../include/main.h"

// Generic Fisher–Yates shuffle
int findSubsetSize(int subsetSize)
{
    int size = sqrt(subsetSize);
    return size;
}

void fisher_yates_shuffle(void **array, size_t n)
{
    srand((unsigned int) time(NULL));

    for (size_t i = n - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);

        // Swap the pointers
        void *tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }

    return;
}

Dataset* createSubset(Dataset* dataset, int subsetSize)
{
    // we randomly sort the given dataset
    fisher_yates_shuffle(dataset->data, dataset->size);

    Dataset* subset = (Dataset*)malloc(sizeof(Dataset));
    if (!subset)
    {
        fprintf(stderr, "Memory allocation failed for subset dataset struct in Kmeans++\n");
        exit(EXIT_FAILURE);
    }

    subset->size = subsetSize;
    subset->dimension = dataset->dimension;
    subset->data_type = dataset->data_type;

    subset->data = dataset->data;

    return subset;
}

void runKmeans(Dataset* subset, int kclusters)
{
    int n = subset->size;
    int d = subset->dimension;
    int t = 0;  //count of centroids
    int i = 0;  //count of non-centroids
    double distance = 0.0;
    int idx = 0;
    double sumDistances = 0.0;
    double chooseRandomNumber = 0.0;
    double density = 0.0;

    bool* is_centroid = (bool*)calloc(n, sizeof(bool));
    if (!is_centroid)
    {
        fprintf(stderr, "Memory allocation failed for is_centroid\n");
        exit(EXIT_FAILURE);
    }

    double* best_distances_square = (double*)calloc(n, sizeof(double));
    if (!best_distances_square)
    {
        fprintf(stderr, "Memory allocation failed for best_distances_square\n");
        exit(EXIT_FAILURE);
    }

    double* probabilities = (double*)calloc(n, sizeof(double));
    if (!probabilities)
    {
        fprintf(stderr, "Memory allocation failed for probabilities\n");
        exit(EXIT_FAILURE);
    }

    if(subset->data_type == DATA_TYPE_FLOAT)
    {
        // Allocate centroid pointers
        float **centroids = malloc(kclusters * sizeof(float*));
        if (!centroids)
        {
            fprintf(stderr, "Memory allocation failed for centroids\n");
            exit(EXIT_FAILURE);
        }


        // We find the first cluster randomly from our dataset
        idx = rand() % n;
        centroids[t] = (float*)subset->data[idx];
        is_centroid[idx] = 1;
        t++;

        for(t; t <= kclusters; t++)
        {
            // initialize best_distances_square array with -1
            for(int b = 0; b < n; b++)
            {
                best_distances_square[b] = -1.0;
            }

            for (int i = 0; i < n; i++)
            {
                // skip if this vector is one of the centroids
                if (is_centroid[i])
                    continue;

                float* vec = (float*)subset->data[i];
                double best_dist = DBL_MAX;

                for (int l = 0; l < t; l++)
                {
                    float* centroid = (float*)centroids[l];
                    double dist = euclidean_distance_float_ivfflat(vec, centroid, d);

                    if (dist < best_dist)
                        best_dist = dist;  //D(i)
                }

                // We have found the best distance for i non-centroid
                sumDistances += best_dist * best_dist;

                // We want to save the best_dist for this point
                best_distances_square[i] = best_dist * best_dist;
            }

            // choose the next cluster vector
            // we have to calculate the probability for each non-centroid point

            for (int i = 0; i < n; i++)
            {
                // skip if this vector is one of the centroids
                if (is_centroid[i])
                    continue;

                probabilities[i] = best_distances_square[i] / sumDistances;
            }

            // Now we have found all of the probabilities
            // Next step is to choose a random number between [0,1]
            chooseRandomNumber = (double)rand() / (double)RAND_MAX;

            for (int i = 0; i < n; i++)
            {
                // skip if this vector is one of the centroids
                if (is_centroid[i])
                    continue;

                density += probabilities[i];

                if(density >= chooseRandomNumber)
                {
                    centroids[t] = (float*)subset->data[i];
                    is_centroid[i] = 1;

                    break;
                }
            }
        }

        // We have our clusters!
        // print them to see if we are correct!
        for(int dx = 0; dx < kclusters; dx++)
        {
            printf("centroid[%d] = {%f,%f}\n", dx, centroids[dx][0], centroids[dx][1]);
        }


    }
    else if(subset->data_type == DATA_TYPE_INT)
    {
        printf("int situation\n");
        // int* i_centroid = (int*)centroid;
        // t++;

        // distance = euclidean_distance_int_ivfflat();
    }
    else
    {
        perror("Dataset type not defined, something is wrong");
        exit(EXIT_FAILURE);
    }

    return;
}

void lloydAlgorithm(void** subset)
{
    //choose a random element to start as a centroid
}

void ivfflat_init(Dataset* dataset, int kclusters)
{
    // int subsetSize = findSubsetSize(dataset->size);
    Dataset* subset = createSubset(dataset, 10); //produces the X'

    // we have to initialize the k centroids
    runKmeans(subset, kclusters);

    // lloydAlgorithm();

    return;
}