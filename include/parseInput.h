#ifndef PARSEINPUT_H
#define PARSEINPUT_H

// Enumeration for algorithm types: LSH, Hypercube, IVFFlat, IVFPQ and none
typedef enum
{
    ALG_NONE,
    ALG_LSH,
    ALG_HYPERCUBE,
    ALG_IVFFLAT,
    ALG_IVFPQ
} AlgorithmType;

// Enumeration for dataset types: MNIST, SIFT and none
typedef enum
{
    DATA_EXP,
    DATA_MNIST,
    DATA_SIFT
} DatasetType;

// Structure to hold all search parameters
typedef struct SearchParams
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

// Function to parse command-line arguments and populate SearchParams structure
int parse_arguments(int argc, char **argv, SearchParams *params);

// Function to print usage instructions
void print_usage(void);

#endif