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
    float **points;      // array of pointers to assigned vectors
} InvertedList;

typedef struct
{
    int k;                  // number of clusters
    int d;                  // dimensionality
    float **centroids;      // centroid vectors
    InvertedList *lists;    // one list per cluster
} IVFFlatIndex;


void add_point_to_list(InvertedList *list, float *point, int cluster_id);
void assign_points_to_clusters(IVFFlatIndex *index, float **dataset, int n, int *assignments);





int findSubsetSize(int subsetSize);

void fisher_yates_shuffle(void **array, size_t n);
Dataset* createSubset(Dataset* dataset, int subsetSize);

centroidInfo* runKmeans(Dataset* dataset, int kclusters);
void lloydAlgorithm(Dataset* dataset, int kclusters);


void ivfflat_init(Dataset* dataset, int kclusters);

#endif