#include "../include/main.h"


void add_point_to_list(InvertedList *list, float *point, int point_id, int cluster_id)
{
    if (list->count == list->capacity) {
        list->capacity = (list->capacity == 0) ? 16 : list->capacity * 2;
        list->points = realloc(list->points, list->capacity * sizeof(float *));
        list->point_ids = realloc(list->point_ids, list->capacity * sizeof(int));
        if (!list->points || !list->point_ids) {
            perror("realloc failed in add_point_to_list");
            exit(EXIT_FAILURE);
        }
    }

    list->points[list->count] = point;
    list->point_ids[list->count] = point_id;
    list->count++;
    list->cluster_id = cluster_id;
}


void clear_lists(IVFFlatIndex *index)
{
    for (int t = 0; t < index->k; t++)
    {
        index->lists[t].count = 0;  // keep memory, just reset counts
    }
}

void assign_points_to_clusters(IVFFlatIndex *index, float **dataset, int start, int end) {
    clear_lists(index);
    for (int i = start; i < end; i++) {
        float *vec = dataset[i];
        double best_dist = DBL_MAX;
        int best_cluster = -1;

        for (int t = 0; t < index->k; t++) {
            double dist = euclidean_distance_float_ivfflat(vec, index->centroids[t], index->d);
            if (dist < best_dist) {
                best_dist = dist;
                best_cluster = t;
            }
        }
        add_point_to_list(&index->lists[best_cluster], vec, i, best_cluster);
    }
}




bool recompute_centroids(IVFFlatIndex *index, int d, double epsilon)
{
    bool changed = false;

    for (int t = 0; t < index->k; t++) {
        InvertedList *list = &index->lists[t];
        if (list->count == 0) continue; // empty cluster

        float *new_centroid = calloc(d, sizeof(float));
        if (!new_centroid) { perror("calloc failed in recompute_centroids"); exit(EXIT_FAILURE); }

        for (int i = 0; i < list->count; i++) {
            float *vec = list->points[i];
            for (int j = 0; j < d; j++)
                new_centroid[j] += vec[j];
        }
        for (int j = 0; j < d; j++)
            new_centroid[j] /= list->count;

        // Check how much the centroid moved
        double shift = euclidean_distance_float_ivfflat(index->centroids[t], new_centroid, d);
        if (shift > epsilon)
            changed = true;

        // Replace old centroid
        free(index->centroids[t]);
        index->centroids[t] = new_centroid;
    }

    return changed;
}

// Generic Fisher–Yates shuffle
int findSubsetSize(int subsetSize)
{
    int size = sqrt(subsetSize);
    // printf("Kmeans++ will use subset size of: %d\n", size);
    // if (size <= 0) size = 1;
    return size;
}

void fisher_yates_shuffle(void **array, size_t n)
{
    // srand((unsigned int) time(NULL));

    // if (n <= 1) return;
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

void printClusters(float** centroids, int kclusters, int d)
{
    for(int dx = 0; dx < kclusters; dx++)
    {
        printf("centroid[%d] = { ", dx);
        for(int x = 0; x < d; x++)
        {
            printf("%f, ", centroids[dx][x]);
        }
        printf("}\n");
    }
}

centroidInfo* runKmeans(Dataset* subset, int kclusters)
{
    int n = subset->size;
    printf("From the subset size: %d we want to find %d centroids!\n", n, kclusters);
    int d = subset->dimension;
    int t = 0;  //count of centroids
    int i = 0;  //count of non-centroids
    double distance = 0.0;
    int idx = 0;
    double sumDistances = 0.0;
    double chooseRandomNumber = 0.0;
    double density = 0.0;

    centroidInfo* info = (centroidInfo*)malloc(sizeof(centroidInfo));
    if (!info)
    {
        fprintf(stderr, "Memory allocation failed for centroid info\n");
        exit(EXIT_FAILURE);
    }

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
        //FIRST OF ALL, EDGE CASE
        // IF kclusters > n (subset->size), then all of the points
        //inside of the subset->size are clusters so let's do that and then return
        if(kclusters > n)
        {
            float **centroids = malloc(n * sizeof(float*));
            if (!centroids)
            {
                fprintf(stderr, "Memory allocation failed for centroids\n");
                exit(EXIT_FAILURE);
            }

            for(int l = 0; l < n; l++)
            {
                centroids[l] = (float*)malloc(d * sizeof(float));
                if (!centroids[l])
                {
                    fprintf(stderr, "Memory allocation failed for centroid copy\n");
                    exit(EXIT_FAILURE);
                }
                    
                for (int _j = 0; _j < d; ++_j)
                    centroids[l][_j] = ((float*)subset->data[l])[_j];
        
                is_centroid[l] = 1;

            }

            // printf("the clusters are all of the subset dataset!\n");
            // print them to see if we are correct!
            // printClusters(centroids, n, d);

            // free our data

            info->centroids = centroids;
            info->is_centroid = is_centroid;

            return info;
        }

        // Allocate centroid pointers
        float **centroids = malloc(kclusters * sizeof(float*));
        if (!centroids)
        {
            fprintf(stderr, "Memory allocation failed for centroids\n");
            exit(EXIT_FAILURE);
        }


        // We find the first cluster randomly from our dataset
        idx = rand() % n;
        /* Allocate and copy the chosen centroid vector instead of pointing directly
        * into the dataset memory. If we keep pointers into the original dataset,
        * later recompute_centroids() will free() centroid pointers and thus free
        * dataset memory, causing use-after-free when the algorithm later reads
        * dataset points. Copying gives centroids ownership of their memory.
        */
        centroids[t] = (float*)malloc(d * sizeof(float));
        if (!centroids[t]) { fprintf(stderr, "Memory allocation failed for centroid copy\n"); exit(EXIT_FAILURE); }
        for (int _j = 0; _j < d; ++_j) centroids[t][_j] = ((float*)subset->data[idx])[_j];
        
        // printf("THE FIRST CENTROID\n");
        // printClusters(centroids, 1, d);
        // printf("THE ABOVE\n");

        
        is_centroid[idx] = 1;
        t++;

        for(t; t < kclusters; t++)
        {
            // initialize best_distances_square array with -1 and reset sumDistances
            for(int b = 0; b < n; b++)
            {
                best_distances_square[b] = -1.0;
            }
            sumDistances = 0.0;

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
                best_distances_square[i] = best_dist * best_dist;
                sumDistances += best_distances_square[i];
            }

            // choose the next cluster vector
            // we have to calculate the probability for each non-centroid point

            for (int i = 0; i < n; i++)
            {
                // skip if this vector is one of the centroids
                if (is_centroid[i])
                    continue;
                probabilities[i] = (sumDistances > 0.0) ? (best_distances_square[i] / sumDistances) : 0.0;
            }

            // Now we have found all of the probabilities
            // Next step is to choose a random number between [0,1]
            chooseRandomNumber = (double)rand() / (double)RAND_MAX;
            density = 0.0;

            for (int i = 0; i < n; i++)
            {
                // skip if this vector is one of the centroids
                if (is_centroid[i])
                    continue;

                density += probabilities[i];

                if(density >= chooseRandomNumber)
                {
                    /* Copy the chosen centroid instead of pointing into dataset memory */
                    centroids[t] = (float*)malloc(d * sizeof(float));
                    if (!centroids[t]) { fprintf(stderr, "Memory allocation failed for centroid copy\n"); exit(EXIT_FAILURE); }
                    for (int _j = 0; _j < d; ++_j) centroids[t][_j] = ((float*)subset->data[i])[_j];
                    is_centroid[i] = 1;

                    break;
                }
            }
        }

        // We have our clusters!
        // print them to see if we are correct!
        // printClusters(centroids, kclusters, d);
        // printf("out of print\n");
        // free our data

        info->centroids = centroids;
        info->is_centroid = is_centroid;
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

    return info;
}

IVFFlatIndex* lloydAlgorithm(Dataset* subset, int kclusters)
{
    // ERROR TO DO, FLOATS AND INTS, NOW IT IS RUNNING WITH FLOAT
    centroidInfo *info = runKmeans(subset, kclusters);
    int max_iters = 50;
    double epsilon = 1e-4;

    //take care of the edge case
    if(kclusters > subset->size)
        kclusters = subset->size;

    // Create IVFFlat index structure
    IVFFlatIndex* index = malloc(sizeof(IVFFlatIndex));
    index->k = kclusters;
    index->d = subset->dimension;
    index->centroids = info->centroids;
    index->lists = calloc(kclusters, sizeof(InvertedList));
    if (!index->lists) { perror("calloc lists"); exit(EXIT_FAILURE); }
    for (int t = 0; t < kclusters; ++t) {
        index->lists[t].points = NULL;
        index->lists[t].point_ids = NULL;
        index->lists[t].count = 0;
        index->lists[t].capacity = 0;
        index->lists[t].cluster_id = t;
    }

    for (int iter = 0; iter < max_iters; iter++)
    {
        assign_points_to_clusters(index, (float **)subset->data, 0, subset->size);
        bool changed = recompute_centroids(index, subset->dimension, epsilon);

        // printf("Iteration %d done.\n", iter + 1);

        // /* Print centroids for this iteration for debugging */
        // for (int ct = 0; ct < index->k; ++ct) {
        //     float *c = index->centroids[ct];
        //     if (c)
        //         printf("  centroid[%d] = {%.6f, %.6f}\n", ct, c[0], c[1]);
        // }

        // for (int t = 0; t < index->k; t++) {
        //     printf("  Cluster %d -> %d points\n", t, index->lists[t].count);
        //     for (int p = 0; p < index->lists[t].count; p++) {
        //         printf("    Point %d: ", p);
        //         float *vec = index->lists[t].points[p];
        //         for (int dim = 0; dim < index->d; dim++) {
        //             printf("%f ", vec[dim]);
        //         }
        //         printf("\n");
        //     }
        // }

        // if (!changed) {
        //     printf("Converged after %d iterations.\n", iter + 1);
        //     break;
        // }
    }

    // clean up temporary memory we no longer need

    free(info->is_centroid);
    free(info);

    // return built index (centroids and lists)
    return index;
}

IVFFlatIndex* ivfflat_init(Dataset* dataset, int kclusters)
{
    int subsetSize = findSubsetSize(dataset->size);
    Dataset* subset = createSubset(dataset, subsetSize); //produces the X'

    // printPartialDataset(subset->size, subset);

    IVFFlatIndex* ivfflat_index =  lloydAlgorithm(subset, kclusters);

    // Now assign the rest of the dataset to the corresponding centroids!
    assign_points_to_clusters(ivfflat_index, (float **)dataset->data, subset->size, dataset->size);
    free(subset);

    return ivfflat_index;
}


void ivfflat_index_lookup(const void *q_void, const struct SearchParams *params, int *approx_neighbors, double *approx_dists, int *approx_count, int **range_neighbors, int *range_count, void *index_data)
{
    IVFFlatIndex* index = (IVFFlatIndex*)index_data;
    const float* q = (const float*)q_void;
    int d = index->d;
    int k = index->k;
    int nprobe = params->nprobe;
    int N = params->N;  // number of neighbors to return
    int R = params->R;  // range distance for range search

    // --- Step 1: Compute distances from query to all centroids ---
    if (nprobe > k) nprobe = k;  // cap nprobe to k
    double* centroid_dists = malloc(nprobe * sizeof(double));
    int* centroid_ids = malloc(nprobe * sizeof(int));
    int selected = 0;
    

    for (int i = 0; i < k; i++) {
        double cent = euclidean_distance_float_ivfflat(q, index->centroids[i], d);
        int j = 0;
        if(selected < nprobe)
        {
            j = selected;
            selected++;
        }
        else 
        {
            if(cent >= centroid_dists[nprobe-1])
                continue;
            j = nprobe -1;
        }

        for(j; j>0 && cent < centroid_dists[j-1]; j--)
        {
            centroid_dists[j] = centroid_dists[j-1];
            centroid_ids[j] = centroid_ids[j-1];
        }
        centroid_dists[j] = cent;
        centroid_ids[j] = i;
    }

    // --- Step 2: Initialize result arrays ---
    *approx_count = 0;
    for (int i = 0; i < N; i++) {
        approx_neighbors[i] = -1;
        approx_dists[i] = INFINITY;
    }

    // --- Step 3: Search within selected (nprobe) clusters ---
    int total_candidates = 0;
    for (int p = 0; p < nprobe && p < k; p++) {
        int cid = centroid_ids[p];
        InvertedList* list = &index->lists[cid];

        for (int i = 0; i < list->count; i++) {
            float* vec = list->points[i];
            double dist = euclidean_distance_float_ivfflat(q, vec, d);
            total_candidates++;

            // Insert into top-N sorted list (like insertion sort)
            if (*approx_count < N || dist < approx_dists[*approx_count - 1]) {
                if (*approx_count < N)
                    (*approx_count)++;

                int j = *approx_count - 1;
                while (j > 0 && dist < approx_dists[j - 1]) {
                    approx_neighbors[j] = approx_neighbors[j - 1];
                    approx_dists[j] = approx_dists[j - 1];
                    j--;
                }
                approx_neighbors[j] = list->point_ids[i];
                approx_dists[j] = dist;
            }

            // --- Optional: Range search support ---
            if (params->range_search && dist <= R) {
                *range_neighbors = realloc(*range_neighbors, (*range_count + 1) * sizeof(int));
                (*range_neighbors)[(*range_count)++] = list->point_ids[i];
            }
        }
    }

    // --- Cleanup ---
    free(centroid_dists);
    free(centroid_ids);
}

void ivfflat_destroy(IVFFlatIndex* index)
{
    if (!index) return;

    // Free all lists (only point_ids and list arrays; the points belong to the dataset)
    for (int t = 0; t < index->k; ++t) {
        if (index->lists[t].points)
            free(index->lists[t].points);
        if (index->lists[t].point_ids)
            free(index->lists[t].point_ids);
    }

    free(index->lists);

    // Free centroids
    if (index->centroids) {
        for (int t = 0; t < index->k; ++t)
            if (index->centroids[t])
                free(index->centroids[t]);
        free(index->centroids);
    }

    free(index);
}
