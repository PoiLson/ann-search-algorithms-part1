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

void assign_points_to_clusters(IVFFlatIndex *index, float **dataset, int n, int *assignments) {
    clear_lists(index);
    for (int i = 0; i < n; i++) {
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

        // if (best_cluster < 0) best_cluster = 0; /* fallback */
        assignments[i] = best_cluster;
        add_point_to_list(&index->lists[best_cluster], vec, i, best_cluster);
    }
}

void assign_ALL_points_to_clusters(IVFFlatIndex *index, float **dataset, int start, int end, int *assignments) {
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

        // if (best_cluster < 0) best_cluster = 0; /* fallback */
        assignments[i] = best_cluster;
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

centroidInfo* runKmeans(Dataset* subset, int kclusters)
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
        for(int dx = 0; dx < kclusters; dx++)
        {
            printf("!!!!!!centroid[%d] = {%f,%f}\n", dx, centroids[dx][0], centroids[dx][1]);
            printf("inside of print\n");
        }
        printf("out of print\n");
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

    int *assignments = calloc(subset->size, sizeof(int));
    if (!assignments) { perror("calloc assignments"); exit(EXIT_FAILURE); }

    for (int iter = 0; iter < max_iters; iter++)
    {
        assign_points_to_clusters(index, (float **)subset->data, subset->size, assignments);
        bool changed = recompute_centroids(index, subset->dimension, epsilon);

        printf("Iteration %d done.\n", iter + 1);

        /* Print centroids for this iteration for debugging */
        for (int ct = 0; ct < index->k; ++ct) {
            float *c = index->centroids[ct];
            if (c)
                printf("  centroid[%d] = {%.6f, %.6f}\n", ct, c[0], c[1]);
        }

        for (int t = 0; t < index->k; t++) {
            printf("  Cluster %d -> %d points\n", t, index->lists[t].count);
            for (int p = 0; p < index->lists[t].count; p++) {
                printf("    Point %d: ", p);
                float *vec = index->lists[t].points[p];
                for (int dim = 0; dim < index->d; dim++) {
                    printf("%f ", vec[dim]);
                }
                printf("\n");
            }
        }

        if (!changed) {
            printf("Converged after %d iterations.\n", iter + 1);
            break;
        }
    }

    /* After convergence, export centroids and assignments to CSV files so Python visualizers
     * can read them. Files are written under Python_Scripts/ for convenience.
     */
    FILE *fc = fopen("Python_Scripts/ivfflat_centroids.csv", "w");
    if (fc) {
        fprintf(fc, "cluster,x,y\n");
        for (int t = 0; t < index->k; ++t) {
            float *c = index->centroids[t];
            if (c)
                fprintf(fc, "%d,%.6f,%.6f\n", t, c[0], c[1]);
        }
        fclose(fc);
    }

    FILE *fa = fopen("Python_Scripts/ivfflat_assignments.csv", "w");
    if (fa) {
        fprintf(fa, "index,x,y,cluster\n");
        for (int i = 0; i < subset->size; ++i) {
            float *v = (float*)subset->data[i];
            int cl = assignments[i];
            fprintf(fa, "%d,%.6f,%.6f,%d\n", i, v[0], v[1], cl);
        }
        fclose(fa);
    }

    /* Print final centroids for clarity before cleanup */
    printf("Final centroids:\n");
    for (int t = 0; t < index->k; ++t) {
        float *c = index->centroids[t];
        if (c)
            printf("  centroid[%d] = {%.6f, %.6f}\n", t, c[0], c[1]);
    }

    // clean up temporary memory we no longer need
    free(assignments);
    free(info->is_centroid);
    free(info);

    // return built index (centroids and lists)
    return index;
}

IVFFlatIndex* ivfflat_init(Dataset* dataset, int kclusters)
{
    int subsetSize = findSubsetSize(dataset->size);
    Dataset* subset = createSubset(dataset, subsetSize); //produces the X'

    IVFFlatIndex* ivfflat_index =  lloydAlgorithm(subset, kclusters);

    // Now assign the rest of the dataset to the corresponding centroids!
    int *assignments = calloc(dataset->size, sizeof(int));
    if (!assignments) { perror("calloc assignments in ivfflat_init"); exit(EXIT_FAILURE); }
    assign_ALL_points_to_clusters(ivfflat_index, (float **)dataset->data, subset->size, dataset->size, assignments);
    free(assignments);

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
    int R = params->N;  // number of neighbors to return

    // --- Step 1: Compute distances from query to all centroids ---
    double* centroid_dists = malloc(k * sizeof(double));
    int* centroid_ids = malloc(k * sizeof(int));

    for (int i = 0; i < k; i++) {
        centroid_dists[i] = euclidean_distance_float_ivfflat(q, index->centroids[i], d);
        centroid_ids[i] = i;
    }

    // --- Step 2: Partial sort to find nprobe closest centroids ---
    // Edge case: k < nprobe
    // So even if someone sets params->nprobe larger than the number of clusters k,
    // the loop automatically limits itself to p < k — no invalid access, no segfault.
    for (int i = 0; i < nprobe && i < k; i++)
    {
        int min_idx = i;
        for (int j = i + 1; j < k; j++)
        {
            if (centroid_dists[j] < centroid_dists[min_idx])
                min_idx = j;
        }
        // Swap centroid_dists
        double tmpd = centroid_dists[i];
        centroid_dists[i] = centroid_dists[min_idx];
        centroid_dists[min_idx] = tmpd;
        // Swap centroid_ids
        int tmpid = centroid_ids[i];
        centroid_ids[i] = centroid_ids[min_idx];
        centroid_ids[min_idx] = tmpid;
    }

    // --- Step 3: Initialize result arrays ---
    *approx_count = 0;
    for (int i = 0; i < R; i++) {
        approx_neighbors[i] = -1;
        approx_dists[i] = INFINITY;
    }

    // --- Step 4: Search within selected (nprobe) clusters ---
    int total_candidates = 0;
    for (int p = 0; p < nprobe && p < k; p++) {
        int cid = centroid_ids[p];
        InvertedList* list = &index->lists[cid];

        for (int i = 0; i < list->count; i++) {
            float* vec = list->points[i];
            double dist = euclidean_distance_float_ivfflat(q, vec, d);
            total_candidates++;

            // Insert into top-R sorted list (like insertion sort)
            if (*approx_count < R || dist < approx_dists[*approx_count - 1]) {
                if (*approx_count < R)
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
            if (params->range_search && dist <= params->R) {
                *range_neighbors = realloc(*range_neighbors, (*range_count + 1) * sizeof(int));
                (*range_neighbors)[(*range_count)++] = list->point_ids[i];
            }
        }
    }

    // --- Step 5: Print candidate diagnostics (for debugging) ---
    printf("IVFFlat lookup: nprobe=%d, total_candidates=%d, returned=%d\n",
           nprobe, total_candidates, *approx_count);

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
