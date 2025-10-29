#ifndef IVFFLAT_H
#define IVFFLAT_H

#include <stdbool.h>
#include <stddef.h>
#include "datasets.h"

typedef struct CentroidInfo
{
    bool *is_centroid;
    float **centroids; // centroids stored as float arrays for both float and int datasets
} centroidInfo;

typedef struct
{
    int cluster_id; // index of the centroid
    int count;      // number of assigned points
    int capacity;   // current capacity for dynamic growth
    void **points;  // actual point vectors
    int *point_ids; // original dataset indices
} InvertedList;

typedef struct
{
    int k;               // number of clusters
    int d;               // dimensionality
    float **centroids;   // centroid vectors
    DataType data_type;  // underlying dataset type (int or float)
    InvertedList *lists; // one list per cluster
} IVFFlatIndex;

// void add_point_to_list(InvertedList *list, void *point, int point_id, int cluster_id);

void assign_points_to_clusters(IVFFlatIndex *index, Dataset *dataset, int start, int end);

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

int findSubsetSize(int subsetSize);

void fisher_yates_shuffle(void **array, size_t n);

Dataset *createSubset(Dataset *dataset, int subsetSize);

centroidInfo *runKmeans(Dataset *dataset, int kclusters);

IVFFlatIndex *lloydAlgorithm(Dataset *subset, int kclusters);

IVFFlatIndex *ivfflat_init(Dataset *dataset, int kclusters);

void ivfflat_index_lookup(const void *q_void, const struct SearchParams *params, int *approx_neighbors, double *approx_dists, int *approx_count, int **range_neighbors, int *range_count, void *index_data);

void ivfflat_destroy(IVFFlatIndex *index);

#endif