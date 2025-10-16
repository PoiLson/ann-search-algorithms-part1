#ifndef LSH_H
#define LSH_H

// Forward declaration
struct LSH;
struct SearchParams;
struct Dataset;

// Data structure for the hash function h(p)
typedef struct 
{
    float *v; // projection vector
    float t;
}LSH_hash_function;

typedef int (*hash_func)(const void* p, const struct LSH* lsh, int table_index, int* ID);
typedef float (*metric_func)(const void* a, const void* b);

// Data Structure for the full LSH
typedef struct LSH
{
    int d; // dimension of the input points
    int L; // number of hash tables
    int k; // number of hash functions per table
    float w; // window size
    int table_size; // size of each hash table
    int num_of_buckets; // number of buckets in each hash table, maybe (m) in the future TODO
    metric_func distance; // distance function
    LSH_hash_function *hash_params; // array of hash parameters
    int **linear_combinations; // array of r[i][j], i in[L], j in[k] for g(p)
    hash_func *amplified_hash_functions; // array of amplified hash functions
    HashTable *hash_tables; // array of hash tables
} LSH;

// Helper functions for hashing
int hash_func_impl_lsh(const void* p ,const LSH* lsh, int table_index, int* ID);
hash_func amplified_hash_function_lsh(const LSH* lsh, int table_index);
int hash_function_lsh(HashTable ht, void* data, int* ID);

LSH* lsh_init(const struct SearchParams* params, const struct Dataset* dataset);
void lsh_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                     int** range_neighbors, int* range_count, void* index_data);
void lsh_destroy(struct LSH* lsh);

#endif