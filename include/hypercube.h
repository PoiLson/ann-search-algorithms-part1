#ifndef HYPERCUBE_H
#define HYPERCUBE_H

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
    Hashmap** map; // per-projection hashmap mapping bucket index -> bit
    bin_hash binary_hash_function; // single hash function that computes all k bits
    
    HashTable hash_table; // hash table
} Hypercube;

//-----------------------Helper functions for hashing-----------------------------

static inline bool f(Hashmap** map, int h_ip)
{
    bool* value = hashmap_getValue(*map, h_ip);
    if (value != NULL)
    {
        return *value;
    }
    else
    {
        bool bit = rand() % 2;
        hashmap_insert(*map, h_ip, bit);
        return bit;
    }
}

// Compute the k-bit binary ID for a point in the hypercube
static inline uint64_t hash_func_impl_hyper(const void* p, const Hypercube* hyper, uint64_t *ID)
{
    uint64_t id = 0ULL;   // use unsigned 64-bit to avoid overflow

    for (int i = 0; i < hyper->kproj; i++)
    {
        float func;
        func = dot_product(hyper->hash_params[i].v, p, hyper->d, hyper->data_type);

        // Compute bucket index using floorf for correctness
        float val = (func + hyper->hash_params[i].t) / hyper->w;
        int h_i = (int)floorf(val);

        // Apply 2-universal hash to map h_i -> {0,1}
        bool bit = f(&(hyper->map[i]), h_i);

        // Shift left and add the new bit
        id = (id << 1) | (uint64_t)bit;
    }

    if (ID) *ID = id;
    return id;  // Return full uint64_t (safe for kproj up to 64 bits)
}


//modifies the hash function to be used in the hash table
static inline int hash_function_hyper(HashTable ht, void* data, uint64_t* ID)
{
    //get the hypercube structure the particular hash function belongs to
    Hypercube* hyper_ctx = (Hypercube*)hash_table_get_algorithm_context(ht);

    if (!hyper_ctx) 
    { 
        if (ID) *ID = 0ULL; 
        return 0; 
    }

    // Use the single hash function that computes all k bits
    return hyper_ctx->binary_hash_function(data, hyper_ctx, ID);
}

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