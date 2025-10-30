#ifndef IVFPQ_H
#define IVFPQ_H

#include "datasets.h"
#include "ivfflat.h"

// Product Quantization parameters for each subspace
typedef struct {
    int M;              // number of subspaces (parts)
    int nbits;          // bits per subspace (typically 8, so 256 centroids per subspace)
    int s;              // number of centroids per subspace (s = 2^nbits)
    int d_sub;          // dimensionality of each subspace (d/M)
    float ***subspace_centroids;  // [M][s][d_sub] - centroids for each subspace
} PQConfig;

// Inverted list entry for IVFPQ (stores compressed PQ codes instead of full vectors)
typedef struct {
    int point_id;       // original dataset index
    uint8_t *pq_code;   // compressed representation: M codes, each indexing a subspace centroid
} IVFPQEntry;

// Inverted list for IVFPQ
typedef struct {
    int cluster_id;
    int count;
    int capacity;
    IVFPQEntry *entries;  // array of compressed entries
} IVFPQList;

// IVFPQ Index structure
typedef struct {
    int k;                    // number of coarse clusters
    int d;                    // original dimensionality
    float **centroids;        // coarse centroids (same as IVFFlat)
    DataType data_type;       // underlying dataset type
    IVFPQList *lists;         // one list per cluster
    PQConfig pq;              // product quantization configuration
} IVFPQIndex;

// Initialize IVFPQ index
IVFPQIndex* ivfpq_init(Dataset *dataset, int k_clusters, int M, int nbits);

// Lookup using IVFPQ (asymmetric distance computation)
void ivfpq_index_lookup(const void *q_void, const struct SearchParams *params, 
                        int *approx_neighbors, double *approx_dists, int *approx_count,
                        int **range_neighbors, int *range_count, void *index_data);

// Destroy IVFPQ index
void ivfpq_destroy(IVFPQIndex *index);

#endif