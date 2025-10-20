#ifndef HYPERCUBE_H
#define HYPERCUBE_H

// Forward declaration
struct Hypercube;

typedef struct Hypercube_hash_function
{
    float *v; // projection vector
    float t;
} Hypercube_hash_function;

typedef int (*bin_hash)(const void* p, const struct Hypercube* hyper, int *ID);

typedef struct Hypercube
{
    int d; // dimension of the input points
    int kproj; // number of projections
    int M; // max number of points to check
    float w; // window size
    int probes; // number of probes
    metric_func distance; // distance function
    Hypercube_hash_function *hash_params; // array of hash parameters
    Hashmap** map; // array of pointers to hashmaps for f functions
    bin_hash *binary_hash_functions; // array of binary hash functions
    
    HashTable hash_table; // hash table
} Hypercube;

// Helper functions for hashing
bool f(Hashmap** map, int h_ip);
int hash_func_impl_hyper(const void* p, const Hypercube* hyper, int *ID);
int hash_hash(const Hypercube* hyper, int index);
int hash_function_hyper(HashTable ht, void* data, int* ID);


Hypercube* hyper_init(const struct SearchParams* params, const struct Dataset* dataset);
void hyper_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                        int** range_neighbors, int* range_count, void* index_data);
void hyper_destroy(struct Hypercube* hyper);

#endif