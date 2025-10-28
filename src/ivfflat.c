#include "../include/main.h"

// unified helper: distance between a dataset point (float or int) and a float centroid
static inline double distance_point_to_centroid(const void *point, int data_type, const float *centroid, int d)
{
    if (data_type == DATA_TYPE_FLOAT)
    {
        return euclidean_distance_float_ivfflat((const float *)point, centroid, d);
    }
    else
    {
        return euclidean_distance_int_to_float((const int *)point, centroid, d);
    }
}

static inline void add_point_to_list(InvertedList *list, void *point, int point_id, int cluster_id)
{
    if (list->count == list->capacity)
    {
        list->capacity = (list->capacity == 0) ? 128 : list->capacity * 2;
        list->points = realloc(list->points, list->capacity * sizeof(void *));
        list->point_ids = realloc(list->point_ids, list->capacity * sizeof(int));
        if (!list->points || !list->point_ids)
        {
            perror("realloc failed in add_point_to_list");
            exit(EXIT_FAILURE);
        }
    }
    list->points[list->count] = point;
    list->point_ids[list->count] = point_id;
    list->count++;
    list->cluster_id = cluster_id;
}

static inline void clear_lists(IVFFlatIndex *index)
{
    for (int t = 0; t < index->k; t++)
    {
        index->lists[t].count = 0; // keep memory, just reset counts
    }
}

static inline void assign_points_to_clusters(IVFFlatIndex *index, Dataset *dataset, int start, int end)
{
    clear_lists(index);
    for (int i = start; i < end; i++)
    {
        void *vec = dataset->data[i];
        double best_dist = DBL_MAX;
        int best_cluster = -1;

        for (int t = 0; t < index->k; t++)
        {
            double dist = distance_point_to_centroid(vec, dataset->data_type, index->centroids[t], index->d);
            if (dist < best_dist)
            {
                best_dist = dist;
                best_cluster = t;
            }
        }
        add_point_to_list(&index->lists[best_cluster], vec, i, best_cluster);
    }
}

static inline bool recompute_centroids(IVFFlatIndex *index, int d, double epsilon)
{
    bool changed = false;

    for (int t = 0; t < index->k; t++)
    {
        InvertedList *list = &index->lists[t];
        if (list->count == 0)
            continue; // empty cluster

        float *new_centroid = calloc(d, sizeof(float));
        if (!new_centroid)
        {
            perror("calloc failed in recompute_centroids");
            exit(EXIT_FAILURE);
        }
        if (index->data_type == DATA_TYPE_FLOAT)
        {
            for (int i = 0; i < list->count; i++)
            {
                float *vec = (float *)list->points[i];
                for (int j = 0; j < d; j++)
                    new_centroid[j] += vec[j];
            }
        }
        else
        {
            for (int i = 0; i < list->count; i++)
            {
                int *ivec = (int *)list->points[i];
                for (int j = 0; j < d; j++)
                    new_centroid[j] += (float)ivec[j];
            }
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
    return size;
}

void fisher_yates_shuffle(void **array, size_t n)
{
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

Dataset *createSubset(Dataset *dataset, int subsetSize)
{
    // we randomly sort the given dataset
    fisher_yates_shuffle(dataset->data, dataset->size);

    Dataset *subset = (Dataset *)malloc(sizeof(Dataset));
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

void printClusters(float **centroids, int kclusters, int d)
{
    for (int dx = 0; dx < kclusters; dx++)
    {
        printf("centroid[%d] = { ", dx);
        for (int x = 0; x < d; x++)
        {
            printf("%f, ", centroids[dx][x]);
        }
        printf("}\n");
    }
}

centroidInfo *runKmeans(Dataset *subset, int kclusters)
{
    int n = subset->size;
    printf("From the subset size: %d we want to find %d centroids!\n", n, kclusters);
    int d = subset->dimension;
    int t = 0; // count of centroids
    int i = 0; // count of non-centroids
    double distance = 0.0;
    int idx = 0;
    double sumDistances = 0.0;
    double chooseRandomNumber = 0.0;
    double density = 0.0;

    centroidInfo *info = (centroidInfo *)malloc(sizeof(centroidInfo));
    if (!info)
    {
        fprintf(stderr, "Memory allocation failed for centroid info\n");
        exit(EXIT_FAILURE);
    }

    bool *is_centroid = (bool *)calloc(n, sizeof(bool));
    if (!is_centroid)
    {
        fprintf(stderr, "Memory allocation failed for is_centroid\n");
        exit(EXIT_FAILURE);
    }

    double *best_distances_square = (double *)calloc(n, sizeof(double));
    if (!best_distances_square)
    {
        fprintf(stderr, "Memory allocation failed for best_distances_square\n");
        exit(EXIT_FAILURE);
    }

    double *probabilities = (double *)calloc(n, sizeof(double));
    if (!probabilities)
    {
        fprintf(stderr, "Memory allocation failed for probabilities\n");
        exit(EXIT_FAILURE);
    }

    /* Unified initialization: handle kclusters > n, then KMeans++ selection using
     * distance_point_to_centroid for both float and int subset types. */
    if (kclusters > n)
    {
        float **centroids = malloc(n * sizeof(float *));
        if (!centroids)
        {
            fprintf(stderr, "Memory allocation failed for centroids\n");
            exit(EXIT_FAILURE);
        }
        for (int l = 0; l < n; l++)
        {
            centroids[l] = (float *)malloc(d * sizeof(float));
            if (!centroids[l])
            {
                fprintf(stderr, "Memory allocation failed for centroid copy\n");
                exit(EXIT_FAILURE);
            }
            if (subset->data_type == DATA_TYPE_FLOAT)
            {
                float *src = (float *)subset->data[l];
                for (int _j = 0; _j < d; ++_j)
                    centroids[l][_j] = src[_j];
            }
            else
            {
                int *src = (int *)subset->data[l];
                for (int _j = 0; _j < d; ++_j)
                    centroids[l][_j] = (float)src[_j];
            }
            is_centroid[l] = 1;
        }
        info->centroids = centroids;
        info->is_centroid = is_centroid;
        return info;
    }
    float **centroids = malloc(kclusters * sizeof(float *));
    if (!centroids)
    {
        fprintf(stderr, "Memory allocation failed for centroids\n");
        exit(EXIT_FAILURE);
    }
    idx = rand() % n;
    centroids[t] = (float *)malloc(d * sizeof(float));
    if (!centroids[t])
    {
        fprintf(stderr, "Memory allocation failed for centroid copy\n");
        exit(EXIT_FAILURE);
    }
    if (subset->data_type == DATA_TYPE_FLOAT)
    {
        float *src = (float *)subset->data[idx];
        for (int _j = 0; _j < d; ++_j)
            centroids[t][_j] = src[_j];
    }
    else
    {
        int *src = (int *)subset->data[idx];
        for (int _j = 0; _j < d; ++_j)
            centroids[t][_j] = (float)src[_j];
    }
    is_centroid[idx] = 1;
    t++;
    for (; t < kclusters; t++)
    {
        for (int b = 0; b < n; b++)
            best_distances_square[b] = -1.0;
        sumDistances = 0.0;
        for (int ii = 0; ii < n; ii++)
        {
            if (is_centroid[ii])
                continue;
            void *vec = subset->data[ii];
            double best_dist = DBL_MAX;
            for (int l = 0; l < t; l++)
            {
                double dist = distance_point_to_centroid(vec, subset->data_type, centroids[l], d);
                if (dist < best_dist)
                    best_dist = dist;
            }
            best_distances_square[ii] = best_dist * best_dist;
            sumDistances += best_distances_square[ii];
        }

        for (int ii = 0; ii < n; ii++)
        {
            if (is_centroid[ii])
                continue;
            probabilities[ii] = (sumDistances > 0.0) ? (best_distances_square[ii] / sumDistances) : 0.0;
        }

        chooseRandomNumber = (double)rand() / (double)RAND_MAX;
        density = 0.0;
        for (int ii = 0; ii < n; ii++)
        {
            if (is_centroid[ii])
                continue;
            density += probabilities[ii];
            if (density >= chooseRandomNumber)
            {
                centroids[t] = (float *)malloc(d * sizeof(float));
                if (!centroids[t])
                {
                    fprintf(stderr, "Memory allocation failed for centroid copy\n");
                    exit(EXIT_FAILURE);
                }
                if (subset->data_type == DATA_TYPE_FLOAT)
                {
                    float *src = (float *)subset->data[ii];
                    for (int _j = 0; _j < d; ++_j)
                        centroids[t][_j] = src[_j];
                }
                else
                {
                    int *src = (int *)subset->data[ii];
                    for (int _j = 0; _j < d; ++_j)
                        centroids[t][_j] = (float)src[_j];
                }
                is_centroid[ii] = 1;
                break;
            }
        }
    }

    info->centroids = centroids;
    info->is_centroid = is_centroid;

    return info;
}

IVFFlatIndex *lloydAlgorithm(Dataset *subset, int kclusters)
{
    // ERROR TO DO, FLOATS AND INTS, NOW IT IS RUNNING WITH FLOAT
    centroidInfo *info = runKmeans(subset, kclusters);
    int max_iters = 50;
    double epsilon = 1e-4;

    // take care of the edge case
    if (kclusters > subset->size)
        kclusters = subset->size;

    // Create IVFFlat index structure
    IVFFlatIndex *index = malloc(sizeof(IVFFlatIndex));
    index->k = kclusters;
    index->d = subset->dimension;
    index->centroids = info->centroids;
    index->data_type = subset->data_type;
    index->lists = calloc(kclusters, sizeof(InvertedList));
    if (!index->lists)
    {
        perror("calloc lists");
        exit(EXIT_FAILURE);
    }
    for (int t = 0; t < kclusters; ++t)
    {
        index->lists[t].points = NULL;
        index->lists[t].point_ids = NULL;
        index->lists[t].count = 0;
        index->lists[t].capacity = 0;
        index->lists[t].cluster_id = t;
    }

    for (int iter = 0; iter < max_iters; iter++)
    {
        assign_points_to_clusters(index, subset, 0, subset->size);
        bool changed = recompute_centroids(index, subset->dimension, epsilon);
    }

    // clean up temporary memory we no longer need
    free(info->is_centroid);
    free(info);

    // return built index (centroids and lists)
    return index;
}

IVFFlatIndex *ivfflat_init(Dataset *dataset, int kclusters)
{
    int subsetSize = findSubsetSize(dataset->size);
    Dataset *subset = createSubset(dataset, subsetSize); // produces the X'

    // printPartialDataset(subset->size, subset);

    IVFFlatIndex *ivfflat_index = lloydAlgorithm(subset, kclusters);

    // Now assign the rest of the dataset to the corresponding centroids!
    assign_points_to_clusters(ivfflat_index, dataset, subset->size, dataset->size);
    free(subset);

    return ivfflat_index;
}

void ivfflat_index_lookup(const void *q_void, const struct SearchParams *params, int *approx_neighbors, double *approx_dists, int *approx_count, int **range_neighbors, int *range_count, void *index_data)
{
    IVFFlatIndex *index = (IVFFlatIndex *)index_data;
    int d = index->d;
    int k = index->k;
    int nprobe = params->nprobe;
    int N = params->N; // number of neighbors to return
    int R = params->R; // range distance for range search

    // --- Step 1: Compute distances from query to all centroids ---
    if (nprobe > k)
        nprobe = k; // cap nprobe to k
    double *centroid_dists = malloc(nprobe * sizeof(double));
    int *centroid_ids = malloc(nprobe * sizeof(int));
    int selected = 0;

  
    const float *qf = NULL;
    const int *qi = NULL;
    if (index->data_type == DATA_TYPE_FLOAT)
        qf = (const float *)q_void;
    else
        qi = (const int *)q_void;

    for (int i = 0; i < k; i++)
    {
        double cent;
        if (qf)
            cent = euclidean_distance_float_ivfflat(qf, index->centroids[i], d);
        else
            cent = euclidean_distance_int_to_float(qi, index->centroids[i], d);

        int j = 0;
        if (selected < nprobe)
        {
            j = selected;
            selected++;
        }
        else
        {
            if (cent >= centroid_dists[nprobe - 1])
                continue;
            j = nprobe - 1;
        }

        for (; j > 0 && cent < centroid_dists[j - 1]; j--)
        {
            centroid_dists[j] = centroid_dists[j - 1];
            centroid_ids[j] = centroid_ids[j - 1];
        }
        centroid_dists[j] = cent;
        centroid_ids[j] = i;
    }

    // --- Step 2: Initialize result arrays ---
    *approx_count = 0;
    for (int i = 0; i < N; i++)
    {
        approx_neighbors[i] = -1;
        approx_dists[i] = INFINITY;
    }

    // --- Step 3: Search within selected (nprobe) clusters ---
    int total_candidates = 0;
    for (int p = 0; p < nprobe && p < k; p++)
    {
        int cid = centroid_ids[p];
        InvertedList *list = &index->lists[cid];

        for (int i = 0; i < list->count; i++)
        {
            void *vec = list->points[i];
            double dist;

            if (index->data_type == DATA_TYPE_FLOAT)
            {
                const float *p = (const float *)vec;
                dist = euclidean_distance_float_ivfflat(qf, p, d);
            }
            else
            {
                const int *p = (const int *)vec;
                dist = (double)euclidean_distance_int((const void *)qi, (const void *)p, d);
            }

            total_candidates++;

            // Insert into top-N sorted list (like insertion sort)
            if (*approx_count < N || dist < approx_dists[*approx_count - 1])
            {
                if (*approx_count < N)
                    (*approx_count)++;

                int j = *approx_count - 1;
                while (j > 0 && dist < approx_dists[j - 1])
                {
                    approx_neighbors[j] = approx_neighbors[j - 1];
                    approx_dists[j] = approx_dists[j - 1];
                    j--;
                }
                approx_neighbors[j] = list->point_ids[i];
                approx_dists[j] = dist;
            }
            // --- Optional: Range search support ---
            if (params->range_search && dist <= R)
            {
                *range_neighbors = realloc(*range_neighbors, (*range_count + 1) * sizeof(int));
                (*range_neighbors)[(*range_count)++] = list->point_ids[i];
            }
        }
    }

    // --- Cleanup ---
    free(centroid_dists);
    free(centroid_ids);
}

void ivfflat_destroy(IVFFlatIndex *index)
{
    if (!index)
        return;

    // Free all lists (only point_ids and list arrays; the points belong to the dataset)
    for (int t = 0; t < index->k; ++t)
    {
        if (index->lists[t].points)
            free(index->lists[t].points);
        if (index->lists[t].point_ids)
            free(index->lists[t].point_ids);
    }

    free(index->lists);

    // Free centroids
    if (index->centroids)
    {
        for (int t = 0; t < index->k; ++t)
            if (index->centroids[t])
                free(index->centroids[t]);
        free(index->centroids);
    }

    free(index);
}
