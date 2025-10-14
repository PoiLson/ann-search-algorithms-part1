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
typedef double (*metric_func)(const void* a, const void* b);

// Data Structure for the full LSH
typedef struct LSH
{
    int d; // dimension of the input points
    int L; // number of hash tables
    int k; // number of hash functions per table
    double w; // window size
    int table_size; // size of each hash table
    int num_of_buckets; // number of buckets in each hash table, maybe (m) in the future TODO
    metric_func distance; // distance function
    LSH_hash_function *hash_params; // array of hash parameters
    int **linear_combinations; // array of r[i][j], i in[L], j in[k] for g(p)
    hash_func *amplified_hash_functions; // array of amplified hash functions
    HashTable *hash_tables; // array of hash tables
} LSH;

int hash_func_impl(const void* p ,const LSH* lsh, int table_index, int* ID);
hash_func amplified_hash_function(const LSH* lsh, int table_index);

int compare_vectors(void* a, void* b);
int hash_function(HashTable ht, void* data, int* ID);
void lsh_init(const struct SearchParams* params, const struct Dataset* dataset);

#endif