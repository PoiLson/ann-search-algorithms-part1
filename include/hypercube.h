#ifndef HYPERCUBE_H
#define HYPERCUBE_H

#include <stdint.h>

// Forward declaration
struct Hypercube;
// Forward declare Hashmap type (defined in include/hashmap.h)
typedef struct Hashmap Hashmap;

//define function pointer
typedef uint64_t (*bin_hash)(const void* p, const struct Hypercube* hyper, uint64_t *ID);

//structure for hypercube hash function parameters
//namely the v normal vector and t offset
typedef struct Hypercube_hash_function
{
    float *v; // projection vector
    float t; // random offset
} Hypercube_hash_function;

// Hypercube structure for holding all relevant parameters and data
typedef struct Hypercube
{
    int d; // dimension of the input points
    int kproj; // number of projections
    int M; // max number of points to check
    float w; // window size
    int probes; // number of probes
    int dataset_size; // size of dataset for visited array allocation
    DataType data_type; // type of data (int or float)
    metric_func distance; // distance function
    Hypercube_hash_function *hash_params; // array of hash parameters
    uint32_t* f_a; // array of a_i coefficients for 2-universal hash (one per bit)
    uint32_t* f_b; // array of b_i offsets for 2-universal hash (one per bit)
    Hashmap** map; // per-projection hashmap mapping bucket index -> bit
    bin_hash binary_hash_function; // single hash function that computes all k bits
    
    HashTable hash_table; // hash table
} Hypercube;

//-----------------------Helper functions for hashing----------------------------

// f function to map h_i to {0,1} using 2-universal hashing for balanced bits
// static inline bool f(uint32_t a, uint32_t b, int h_i)
// {
//     return h_i & 1;
// }

//defines the hypercube hash function
static uint64_t hash_func_impl_hyper(const void* p, const Hypercube* hyper, uint64_t *ID);

//modifies the hash function to be used in the hash table
int hash_function_hyper(HashTable ht, void* data, uint64_t* ID);

//--------------------------Hypercube main functions-----------------------------

// initializes the hypercube structure, allocates memory, sets parameters, and stores ddata points
Hypercube* hyper_init(const struct SearchParams* params, const struct Dataset* dataset);

// performs approximate nearest neighbor search using the hypercube index
void hyper_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors,
                        double* approx_dists, int* approx_count, int** range_neighbors, 
                        int* range_count, void* index_data);

// frees all allocated memory associated with the hypercube structure
void hyper_destroy(struct Hypercube* hyper);

#endif