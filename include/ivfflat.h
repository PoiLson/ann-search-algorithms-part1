#ifndef IVFFLAT_H
#define IVFFLAT_H


typedef struct CentroidInfo
{
    bool* is_centroid;
    float** centroids;
} centroidInfo;

typedef struct
{
    int cluster_id;      // index of the centroid
    int count;           // number of assigned points
    int capacity;        // current capacity for dynamic growth
    float **points;     // actual point vectors
    int *point_ids;     // original dataset indices
} InvertedList;

typedef struct
{
    int k;                  // number of clusters
    int d;                  // dimensionality
    float **centroids;      // centroid vectors
    InvertedList *lists;    // one list per cluster
} IVFFlatIndex;


void add_point_to_list(InvertedList *list, float *point, int point_id, int cluster_id);

void assign_points_to_clusters(IVFFlatIndex *index, float **dataset, int start, int end);




int findSubsetSize(int subsetSize);

void fisher_yates_shuffle(void **array, size_t n);
Dataset* createSubset(Dataset* dataset, int subsetSize);

centroidInfo* runKmeans(Dataset* dataset, int kclusters);
IVFFlatIndex* lloydAlgorithm(Dataset* subset, int kclusters);


IVFFlatIndex* ivfflat_init(Dataset* dataset, int kclusters);

void ivfflat_index_lookup(const void *q_void, const struct SearchParams *params, int *approx_neighbors, double *approx_dists, int *approx_count, int **range_neighbors, int *range_count, void *index_data);

void ivfflat_destroy(IVFFlatIndex* index);

#endif