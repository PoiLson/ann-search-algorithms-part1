#ifndef HYPERCUBE_H
#define HYPERCUBE_H

// Forward declaration
struct Hypercube;

//define function pointer
typedef int (*bin_hash)(const void* p, const struct Hypercube* hyper, int *ID);

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
    metric_func distance; // distance function
    Hypercube_hash_function *hash_params; // array of hash parameters
    Hashmap** map; // array of pointers to hashmaps for f functions
    bin_hash binary_hash_function; // single hash function that computes all k bits
    
    HashTable hash_table; // hash table
} Hypercube;

//-----------------------Helper functions for hashing----------------------------

// f function to map h_i to {0,1} uniformly, using hashmaps
// to store/ access previously computed values
static bool f(Hashmap** map, int h_ip);

//defines the hypercube hash function
static int hash_func_impl_hyper(const void* p, const Hypercube* hyper, int *ID);

//modifies the hash function to be used in the hash table
int hash_function_hyper(HashTable ht, void* data, int* ID);

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