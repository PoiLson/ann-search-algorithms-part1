#ifndef LSH_H
#define LSH_H


// Forward declaration
struct LSH;
struct SearchParams;

// Data structure for the hash function h(p)
typedef struct 
{
    float *v; // projection vector
    float t;
}LSH_hash_function;

typedef int (*hash_func)(const float* p, const struct LSH* lsh, int table_index);
typedef double (*metric_func)(const float* a, const float* b, int d);

// Data Structure for the full LSH
typedef struct LSH
{
    int d; // dimension of the input points
    int L; // number of hash tables
    int k; // number of hash functions per table
    double w; // window size
    int table_size; // size of each hash table
    int num_of_buckets; // number of buckets in each hash table
    metric_func distance; // distance function
    LSH_hash_function *hash_params; // array of hash parameters
    // hash_func *hash_functions; // array of hash functions
    int **linear_combinations; // array of r[i][j], i in[L], j in[k] for g(p)
    hash_func *amplified_hash_functions; // array of amplified hash functions
    HashTable *hash_tables; // array of hash tables



} LSH;



// hash_func h_function(const LSH_hash_function* h, int d, double w);
// int ID_function(const LSH* lsh, const float* p, int hash_index);
// int amplified_hash(const LSH* lsh, const float* p, int table_index);

int hash_func_impl(const float* p ,const LSH* lsh, int table_index);
hash_func amplified_hash_function(const LSH* lsh, int table_index);




int compare_points(void* a, void* b);
int hash_function(HashTable ht, void* key);
void lsh_init(const struct SearchParams* params, int dimension, int dataset_size, float** dataset);


#endif