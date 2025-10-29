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
    int dataset_size; // total number of points in the dataset (for visited array)
    int table_size; // size of each hash table
    uint64_t num_of_buckets; // large modulus M for ID composition (supports up to ~2^64)
    DataType data_type; // type of data (int or float)
    metric_func distance; // distance function
    // Per-table hash parameters: for each table (L), we have k hash functions (v, t)
    // Access pattern: hash_params[table_index][i]
    LSH_hash_function **hash_params; 
    int **linear_combinations_1; // array of r[i][j], i in[L], j in[k] for g(p)
    int **linear_combinations_2; // array of r[i][j], i in[L], j in[k] for g(p)
    
    
    
    HashTable *hash_tables; // array of hash tables
} LSH;



#define R_RANGE (1U << 29)  // values in [1, 2^29]


// ------------------- Helper functions for hashing -----------------------------

// defines the LSH hash function
// g(p) = ID(p) mod table_size
// where ID(p) = sum(h_i(p) * r_i) mod M
// r_i are random integers saved in the LSH struct
// M is the number of buckets also saved in the LSH struct
// h_i are the hash functions saved in the LSH struct
// g() needs to be a function stored in the LSH struct so it needs to return hash_func type
// inline int hash_func_impl_lsh(const void* p ,const LSH* lsh, int table_index, uint64_t* ID);

// inline int hash_function_lsh(HashTable ht, void* data, uint64_t* ID);


// Compute both hash value (table index) and fingerprint (ID)
static inline int hash_func_impl_lsh(const void *p, const LSH *lsh, int table_index, uint64_t *outID)
{
    // The prime modulus for universal hashing
    const uint64_t M = lsh->num_of_buckets; // should be 2^32 - 5

    uint64_t hash_val = 0; // for h₁ → table index
    uint64_t id_val = 0;   // for h₂ → fingerprint

    const LSH_hash_function *table_hash_params = lsh->hash_params[table_index];

    for (int i = 0; i < lsh->k; i++)
    {
        // ---- Compute LSH projection: h_i(v) = floor((a·v + b)/w)
        double func = 0.0;
        if (lsh->data_type == DATA_TYPE_FLOAT)
            func = dot_product_float(table_hash_params[i].v, (const float *)p, lsh->d);
        else
            func = dot_product_float_uint8(table_hash_params[i].v, (const uint8_t *)p, lsh->d);

        double val = (func + (double)table_hash_params[i].t) / (double)lsh->w;
        int64_t h_i = (int64_t)floor(val); // handle negatives properly

        // ---- Universal hashing linear coefficients
        int64_t r1_i = lsh->linear_combinations_1[table_index][i]; // for hash value
        int64_t r2_i = lsh->linear_combinations_2[table_index][i]; // for ID

        // ---- Compute modular products safely in 128-bit space
        __int128 prod1 = (__int128)r1_i * (__int128)h_i;
        __int128 prod2 = (__int128)r2_i * (__int128)h_i;

        // Reduce modulo prime (always non-negative)
        uint64_t mod1 = (uint64_t)((prod1 % (int64_t)M + (int64_t)M) % (int64_t)M);
        uint64_t mod2 = (uint64_t)((prod2 % (int64_t)M + (int64_t)M) % (int64_t)M);

        // ---- Accumulate mod prime
        hash_val = (hash_val + mod1) % M;
        id_val = (id_val + mod2) % M;
    }

    // ---- Output fingerprint (h₂)
    if (outID)
        *outID = id_val; // this is the "h2" in the manual (fingerprint)

    // ---- Final hash table index (h₁)
    int bucket_idx = (int)(hash_val % (uint64_t)lsh->table_size);
    return bucket_idx;
}

// Wrapper function
static inline int hash_function_lsh(HashTable ht, void *data, uint64_t *ID)
{
    LSH *lsh_ctx = (LSH *)hash_table_get_algorithm_context(ht);
    if (!lsh_ctx || lsh_ctx->num_of_buckets == 0)
    {
        if (ID)
            *ID = 0; // safer sentinel than -1 in uint64_t
        return 0;
    }

    int t_idx = hash_table_get_index(ht);
    return hash_func_impl_lsh(data, lsh_ctx, t_idx, ID);
}

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