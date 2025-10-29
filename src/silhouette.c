#include "../include/main.h"

int compute_silhouette_parallel(const IVFFlatIndex *index, const Dataset *dataset, double *per_cluster)
{
    if (!index || !dataset)
    {
        perror("The needed structs are not intialized\n");
        exit(EXIT_FAILURE);
    }

    const int k = index->k;
    const int d = index->d;
    const int n = dataset->size;
    if (k <= 0 || n <= 0)
    {
        perror("kclusters or the dataset size is 0\n");
        exit(EXIT_FAILURE);
    }

    /* ---------- 1. Build cluster_of[point] = cluster index ---------- */
    // Finds from all the points of the dataset the cluster they have
    int *cluster_of = (int *)calloc((size_t)n, sizeof(int));
    if (!cluster_of)
    {
        printf("Error in allocating cluster_of!\n");
        exit(EXIT_FAILURE);
    }

    for (int c = 0; c < k; ++c)
    {
        for (int j = 0; j < index->lists[c].count; ++j)
        {
            int pid = index->lists[c].point_ids[j];
            cluster_of[pid] = c;
        }
    }
    
    /* ---------- 2. Allocate silhouette array (one per point) ---------- */
    double *s = (double *)calloc((size_t)n, sizeof(double));
    if (!s)
    {
        printf("Error allocating space for silhouette arrays\n");
        free(cluster_of);
        return -1;
    }
    
    printf("reached the nested parallization\n");
    /* ---------- 3. Parallel per-point computation ---------- */
    // Another method of parallization, 
    // === NESTED PARALLELISM ===
    #pragma omp parallel
    {
        // Level 1: Distribute clusters across outer threads
        #pragma omp for schedule(dynamic, 1)
        for (int c = 0; c < k; ++c)
        {
            const InvertedList *list = &index->lists[c];

            // Level 2: Parallelize points INSIDE this cluster
            printf("calculating a(i) and b(i)\n");
            #pragma omp parallel for schedule(static)
            for (int j = 0; j < list->count; ++j)
            {
                int pid = list->point_ids[j];
                void *point = dataset->data[pid];

                // a(i): avg dist in same cluster (skip self)
                double a = 0.0;
                for (int k = 0; k < list->count; ++k)
                {
                    if (k == j)
                        continue;

                    // now i am runnning it iwht time make mnist so it is ints we will see
                    a += euclidean_distance_int(point, list->points[k], dataset->dimension);
                }
                a /= (list->count - 1);

                // b(i): min avg dist to other clusters
                double best_distance = DBL_MAX;
                double distance = 0.0;
                int nearest_centroid = -1;

                for (int c2 = 0; c2 < k; ++c2)
                {
                    if (c2 == c || index->lists[c2].count == 0)
                        continue;

                    // now  i am running with mnist so it is integer
                    int* pointForDistance = (int*)point;
                    distance = euclidean_distance_int_to_float(pointForDistance, index->centroids[c2], dataset->dimension);

                    if(distance < best_distance)
                    {
                        best_distance = distance;
                        nearest_centroid = c2;
                    }
                }
                
                if(nearest_centroid == -1)
                {
                    printf("Cannot find any nearest centroid\n");
                    exit(EXIT_FAILURE);
                }

                // Now that we have found the nearest cluster to that point we find the b(i)
                const InvertedList *nearestCentroidList = &index->lists[nearest_centroid];

                double b = 0.0;
                for (int k = 0; k < nearestCentroidList->count; ++k)
                {
                    // now i am runnning it iwht time make mnist so it is ints we will see
                    b += euclidean_distance_int(point, nearestCentroidList->points[k], dataset->dimension);
                }
                b /= nearestCentroidList->count;

                // now we calculate the s(i)
                double si = (a == 0.0 && b == 0.0) ? -2 : (b - a) / fmax(a, b);
                s[pid] = si;
            }
        }
    }
    
    printf("end of parallilzation\n");

    /* ---------- 4. Optional per-cluster averages (parallel) ---------- */
    if (per_cluster)
    {
        #pragma omp parallel for
        for (int c = 0; c < k; ++c)
        {
            const InvertedList *L = &index->lists[c];
            // if this cluster does not have any points assigned ot it it is fundamentally wrong right?
            // so exit if we have this situation?

            double sum = 0.0;
            for (int j = 0; j < L->count; ++j)
            {
                const int pid = L->point_ids[j];
                sum += s[pid];
            }

            per_cluster[c] = sum / L->count;
        }
    }

    free(cluster_of);
    free(s);

    return 1;
}