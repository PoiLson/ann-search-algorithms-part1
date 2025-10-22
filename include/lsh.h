#ifndef LSH_H
#define LSH_H

#include <stdint.h>

// Forward declarations
struct LSH;
struct SearchParams;
struct Dataset;

// Data structure for the hash function h(p)
typedef struct 
{
    float *v; // projection vector
    float t; // random offset
}LSH_hash_function;

// Define function pointer types
typedef int (*hash_func)(const void* p, const struct LSH* lsh, int table_index, uint64_t* ID);
typedef float (*metric_func)(const void* a, const void* b, const int dimension);

// Data Structure for the full LSH
typedef struct LSH
{
    int d; // dimension of the input points
    int L; // number of hash tables
    int k; // number of hash functions per table
    float w; // window size
    int table_size; // size of each hash table
    uint64_t num_of_buckets; // large modulus M for ID composition (supports up to ~2^64)
    metric_func distance; // distance function
    // Per-table hash parameters: for each table (L), we have k hash functions (v, t)
    // Access pattern: hash_params[table_index][i]
    LSH_hash_function **hash_params; 
    int **linear_combinations; // array of r[i][j], i in[L], j in[k] for g(p)
    HashTable *hash_tables; // array of hash tables
} LSH;

// ------------------- Helper functions for hashing -----------------------------

// defines the LSH hash function
// g(p) = ID(p) mod table_size
// where ID(p) = sum(h_i(p) * r_i) mod M
// r_i are random integers saved in the LSH struct
// M is the number of buckets also saved in the LSH struct
// h_i are the hash functions saved in the LSH struct
// g() needs to be a function stored in the LSH struct so it needs to return hash_func type
int hash_func_impl_lsh(const void* p ,const LSH* lsh, int table_index, uint64_t* ID);

int hash_function_lsh(HashTable ht, void* data, uint64_t* ID);


// ------------------------- LSH main functions ---------------------------------

// initializes the LSH structure, allocates memory, sets parameters, and stores data points
LSH* lsh_init(const struct SearchParams* params, const struct Dataset* dataset);

// performs approximate nearest neighbor search using the LSH index
void lsh_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors,
                        double* approx_dists, int* approx_count, int** range_neighbors, 
                        int* range_count, void* index_data);

// frees all allocated memory associated with the LSH structure
void lsh_destroy(struct LSH* lsh);

#endif