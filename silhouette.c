#include "../include/main.h"


// Fast silhouette computation with parallelization and optional sampling
void computeSilhouette(IVFFlatIndex* index, Dataset* dataset) {
    if (!index || !dataset || index->k == 0) {
        return;
    }

    int k = index->k;
    int dim = dataset->dimension;
    int n = dataset->size;
    DataType data_type = dataset->data_type;
    if (k <= 0 || n <= 0)
    {
        perror("kclusters or the dataset size is 0\n");
        exit(EXIT_FAILURE);
    }

    /* ---------- 1. Build cluster_of[point] = cluster index ---------- */
    double *per_cluster = (double *)malloc(index->k * sizeof(double));
    if (!per_cluster)
    {
        printf("Error in allocating per_cluster!\n");
        exit(EXIT_FAILURE);
    }

    // Finds from all the points of the dataset the cluster they have
    int *cluster_of = (int *)calloc((size_t)n, sizeof(int));
    if (!cluster_of)
    {
        printf("Error in allocating cluster_of!\n");
        free(per_cluster);
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
        free(per_cluster);
        exit(EXIT_FAILURE);
    }
    
    printf("reached the nested parallization\n");

    // ===== INTRA-CLUSTER: Compute a(i) using symmetric pairwise distances =====
    // Single-level parallelization over clusters
    #pragma omp parallel for schedule(dynamic)
    for (int c = 0; c < k; c++)
    {
        double sum = 0.0;
        const InvertedList *list = &index->lists[c];
        int cluster_size = list->count;
        // printf("Thread %d starting cluster %d\n", omp_get_thread_num(), c);
        fflush(stdout);
        

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
                a += euclidean_distance(point, list->points[k], dataset->dimension, dataset->data_type, dataset->data_type);
                // printf("a for cluster %d is %f\n", c, a);
            }
            a /= (list->count - 1);

            // b(i): min avg dist to other clusters
            double best_distance = DBL_MAX;
            int nearest_centroid = -1;

            #pragma omp parallel for reduction(min:best_distance)
            for (int c2 = 0; c2 < k; ++c2)
            {
                // printf("Thread %d starting cluster %d\n", omp_get_thread_num(), c);
                if (c2 == c || index->lists[c2].count == 0)
                    continue;

                double distance = 0.0;
                for (int kk = 0; kk < index->lists[c2].count; ++kk)
                    distance += euclidean_distance(point, index->lists[c2].points[kk], dataset->dimension, dataset->data_type, dataset->data_type);

                distance /= index->lists[c2].count;

                if (distance < best_distance)
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

            
            double b = best_distance;

            // now we calculate the s(i)
            double si = (a == 0.0 && b == 0.0) ? -2 : (b - a) / fmax(a, b);
            s[pid] = si;
            sum += si;
        }

        per_cluster[c] = sum / list->count;
        printf("Thread %d end of cluster %d, per cluster: %f\n", omp_get_thread_num(), c, per_cluster[c]);

    
    }

    printf("end of parallilzation\n");

    free(cluster_of);
    free(s);

    return;
}