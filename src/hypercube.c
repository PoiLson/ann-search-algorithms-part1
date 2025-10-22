#include "../include/main.h"


static bool f(Hashmap** map, int h_ip)
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

static int hash_func_impl_hyper(const void* p, const Hypercube* hyper, uint64_t *ID)
{
    int id = 0;
    // Compute each h_i and map to bit using f 
    // Then combine bits into final ID i.e. concatenate bits
    for (int i = 0; i < hyper->kproj; i++)
    {
        // Use int-aware dot product for MNIST integer data
        float func = dot_product_float_int(hyper->hash_params[i].v, p, hyper->d);
        float val = (func + hyper->hash_params[i].t) / hyper->w;
        
        // Fast integer truncation instead of floor()
        // For negative values, (int) truncates toward zero, but floor goes toward -infinity
        // So we need to adjust: if val < 0 and val != integer, subtract 1
        int h_i = (int)val;
        if (val < 0.0f && val != (float)h_i)
            h_i--;

        bool bit = f(&(hyper->map[i]), h_i);
        id = (id << 1) | bit; // Shift left and add the new bit
    }

    if (ID)
        *ID = (uint64_t)id;
    return id;
}

int hash_function_hyper(HashTable ht, void* data, uint64_t* ID)
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



Hypercube* hyper_init(const struct SearchParams* params, const struct Dataset* dataset)
{
    // Allocate memory for Hypercube structure
    // and set parameters
    Hypercube* hyper = (Hypercube*)malloc(sizeof(Hypercube));
    if (!hyper)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    hyper->d = dataset->dimension;
    hyper->kproj = params->kproj;
    hyper->w = params->w;
    hyper->M = params->M;
    hyper->probes = params->probes;
    hyper->dataset_size = dataset->size; ///add it as a func parameter TODO tomorrow
    // Use int-based distance for MNIST integer data
    hyper->distance = euclidean_distance_int;

    // Initialize hash parameters
    hyper->hash_params = (Hypercube_hash_function*)malloc(hyper->kproj * sizeof(Hypercube_hash_function));
    if (!hyper->hash_params)
    {
        hyper_destroy(hyper);
        return NULL;
    }

    // Generate random vectors and offsets for each hash function
    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->hash_params[i].v = (float*)malloc(hyper->d * sizeof(float));
        generate_random_vector(hyper->hash_params[i].v, hyper->d);

        if(!hyper->hash_params[i].v)
        {
            hyper_destroy(hyper);
            exit(EXIT_FAILURE);
        }

        int tmp = 0;
        hyper->hash_params[i].t = uniform_distribution(&tmp, &(hyper->w));
    }

    // Initialize hashmaps for f functions
    hyper->map = (Hashmap**)malloc(hyper->kproj * sizeof(Hashmap*));
    if (!hyper->map)
    {
        hyper_destroy(hyper);
        exit(EXIT_FAILURE);
    }

    // Create a hashmap for each f function
    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->map[i] = hashmap_init(2048);
        if (!hyper->map[i])
        {
            hyper_destroy(hyper);
            exit(EXIT_FAILURE);
        }
    }

    // Set the single binary hash function for hypercube
    // (computes all k bits to form the bucket index)
    hyper->binary_hash_function = hash_func_impl_hyper;

    // Create hash table with algorithm context and index 0
    hyper->hash_table = hash_table_create(1 << hyper->kproj, sizeof(int), NULL, NULL, hash_function_hyper, hyper, 0, &(dataset->dimension));
    if (!hyper->hash_table)
    {
        hyper_destroy(hyper);
        exit(EXIT_FAILURE);
    }

    // Insert dataset points into hash table
    for (int i = 0; i < dataset->size; i++)
    {
        if (!dataset->data || !dataset->data[i])
        {
            printf("Warning: dataset[%d] is NULL\n", i);
            continue;
        }

        // Insert the valid data point into the hash table
        hash_table_insert(hyper->hash_table, &i, dataset->data[i]);
    }
    
    // print the contents of the hash table -- debug only
    //print_hashtable(hyper->hash_table, 1 << hyper->kproj, dataset->dimension);

    return hyper;
}

void hyper_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                        int** range_neighbors, int* range_count, void* index_data)
{
    // Retrieve hypercube structure from context
    struct Hypercube* hyper = (struct Hypercube*)index_data;

    // Allocate visited array for O(1) duplicate detection (instead of O(N) linear scan)
    // This dramatically speeds up queries when examining many candidates
    bool* visited = (bool*)calloc(hyper->dataset_size, sizeof(bool));
    if (!visited) {
        fprintf(stderr, "Failed to allocate visited array\n");
        return;
    }

    // Range search optimization: allocate in chunks to avoid repeated realloc
    int range_capacity = 0;
    const int RANGE_ALLOC_CHUNK = 128; // Grow by 128 entries at a time

    // Compute the bucket index for the query point
    uint64_t q_id;
    int bucket_idx = hyper->binary_hash_function(q, hyper, &q_id);

    // Access the bucket corresponding to the computed index
    int bucket_count = 0;
    const HTEntry* bucket = hash_table_get_bucket_entries(hyper->hash_table, bucket_idx, &bucket_count);
    
    // Compute Hamming distance and probe other buckets if needed
    // Calculate binary vector representation of bucket_idx
    int* bucket_vector = (int*)malloc(hyper->d * sizeof(int));
    for (int i = 0; i < hyper->kproj; i++)
    {
        bucket_vector[i] = (bucket_idx >> (hyper->kproj - 1 - i)) & 1;
    }
    
    int* neighbors = NULL;
    int neighbor_count = hyper->probes;

    // Get neighboring bucket indices based on Hamming distance
    // only the required number of probes
    get_hamming_neighbors(bucket_vector, hyper->probes, hyper->kproj, &neighbors);
    for (int n = 0; n < neighbor_count; n++)
    {
        int neighbor_idx = 0;
        // Access the n-th neighbor from the flat array
        int* current_neighbor = neighbors + (n * hyper->kproj);
        for (int b = 0; b < hyper->kproj; b++)
        {
            neighbor_idx = (neighbor_idx << 1) | current_neighbor[b];
        }
        int neighbor_count_entries = 0;
        const HTEntry* neighbor_bucket = hash_table_get_bucket_entries(hyper->hash_table, neighbor_idx, &neighbor_count_entries);

        // Process the neighbor bucket entries array
        for (int bi = 0; bi < neighbor_count_entries; ++bi)
        {
            int data_idx = *(int*)neighbor_bucket[bi].key;
            void* p = neighbor_bucket[bi].data;

            // O(1) duplicate check using visited array instead of O(N) linear scan
            if (visited[data_idx])
                continue;
            visited[data_idx] = true;

            // Use int-based distance computation for MNIST integer data
            float dist = euclidean_distance_int(q, p, hyper->d);

            // Insert into approx neighbors if appropriate
            if (*approx_count < params->N || dist < approx_dists[*approx_count - 1])
            {
                if (*approx_count < params->N)
                    (*approx_count)++;
                int i = 0;
                // Insert in sorted order
                for (i = *approx_count - 1; i > 0 && (i
                    == 0 || dist < approx_dists[i - 1]); i--)
                {
                    approx_dists[i] = approx_dists[i - 1];
                    approx_neighbors[i] = approx_neighbors[i - 1];
                }
                approx_dists[i] = dist;
                approx_neighbors[i] = data_idx;
            }

            // Range search (visited array already handles deduplication)
            if (params->range_search && dist <= params->R)
            {
                // Allocate in chunks to avoid repeated realloc overhead
                if (*range_count >= range_capacity)
                {
                    range_capacity += RANGE_ALLOC_CHUNK;
                    int* new_range = (int*)realloc(*range_neighbors, range_capacity * sizeof(int));
                    if (!new_range)
                    {
                        fprintf(stderr, "Memory reallocation failed for range_neighbors\n");
                        free(*range_neighbors);
                        *range_neighbors = NULL;
                        *range_count = 0;
                        free(visited);
                        free(bucket_vector);
                        free(neighbors);
                        return;
                    }
                    *range_neighbors = new_range;
                }
                (*range_neighbors)[(*range_count)++] = data_idx;
            }

        }
    }

    // Clean up
    free(visited);
    free(bucket_vector);
    free(neighbors);

}

void hyper_destroy(struct Hypercube* hyper)
{
    if (!hyper) return;

    // Free hash table first
    if (hyper->hash_table)
        hash_table_destroy(hyper->hash_table);

    // Free hash parameters
    if (hyper->hash_params)
    {
        for (int i = 0; i < hyper->kproj; i++)
        {
            if (hyper->hash_params[i].v)
                free(hyper->hash_params[i].v);
        }
        free(hyper->hash_params);
    }

    // Free hashmaps
    if (hyper->map)
    {
        for (int i = 0; i < hyper->kproj; i++)
        {
            if (hyper->map[i])
                hashmap_free(hyper->map[i]);
        }
        free(hyper->map);
    }

    // Finally free the structure itself
    free(hyper);
}
