#ifndef PARSEINPUT_H
#define PARSEINPUT_H

#include "main.h"

typedef enum
{
    ALG_NONE,
    ALG_LSH,
    ALG_HYPERCUBE,
    ALG_IVFFLAT,
    ALG_IVFPQ
} AlgorithmType;

typedef enum
{
    DATA_NONE,
    DATA_MNIST,
    DATA_SIFT
} DatasetType;

typedef struct
{
    // Common parameters
    char dataset_path[256];
    char query_path[256];
    char output_path[256];
    DatasetType dataset_type;
    AlgorithmType algorithm;
    bool range_search;
    int N;          // number of nearest neighbors
    double R;       // search radius

    // LSH params
    int k;
    int L;
    double w;

    // Hypercube params
    int kproj;
    int M;
    int probes;

    // IVFFlat / IVFPQ params
    int kclusters;
    int nprobe;
    unsigned int seed;

    // IVFPQ extra params
    int nbits;
    int M_pq;   // number of subvectors for PQ
} SearchParams;

int parse_arguments(int argc, char **argv, SearchParams *params);
void print_usage(void);

#endif