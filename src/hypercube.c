#include "../include/main.h"

#include <math.h>   // for floorf

// Compute the k-bit binary ID for a point in the hypercube
static uint64_t hash_func_impl_hyper(const void* p, const Hypercube* hyper, uint64_t *ID)
{
    uint64_t id = 0ULL;   // use unsigned 64-bit to avoid overflow

    for (int i = 0; i < hyper->kproj; i++)
    {
        float func;
        if (hyper->data_type == DATA_TYPE_FLOAT)
            func = dot_product_float(hyper->hash_params[i].v, (const float*)p, hyper->d);
        else
            func = dot_product_float_int(hyper->hash_params[i].v, (const int*)p, hyper->d);

        // Compute bucket index using floorf for correctness
        float val = (func + hyper->hash_params[i].t) / hyper->w;
        int h_i = (int)floorf(val);

        // Apply 2-universal hash to map h_i -> {0,1}
        bool bit = f(hyper->f_a[i], hyper->f_b[i], h_i);

        // Shift left and add the new bit
        id = (id << 1) | (uint64_t)bit;
    }

    if (ID) *ID = id;
    return id;  // Return full uint64_t (safe for kproj up to 64 bits)
}


static void hyper_check_f_balance(const Hypercube* hyper, const Dataset* dataset, int sample_size)
{
    const int k = hyper->kproj;
    if (k <= 0) {
        fprintf(stderr, "[diag] No projections (k=0), skipping balance check\n");
        return;
    }

    long long* ones  = (long long*)calloc(k, sizeof(long long));
    long long* zeros = (long long*)calloc(k, sizeof(long long));
    if (!ones || !zeros) {
        free(ones); free(zeros);
        fprintf(stderr, "[diag] Failed to allocate counters for f_i balance\n");
        return;
    }

    int total = dataset->size;
    if (sample_size > 0 && sample_size < total) total = sample_size;

    for (int i = 0; i < total; ++i)
    {
        const void* p = dataset->data[i];
        if (!p) continue; // safety check

        for (int j = 0; j < k; ++j)
        {
            float func;
            if (hyper->data_type == DATA_TYPE_FLOAT)
                func = dot_product_float(hyper->hash_params[j].v, (const float*)p, hyper->d);
            else
                func = dot_product_float_int(hyper->hash_params[j].v, (const int*)p, hyper->d);

            float val = (func + hyper->hash_params[j].t) / hyper->w;
            int h_i = (int)floorf(val);

            bool bit = f(hyper->f_a[j], hyper->f_b[j], h_i);
            if (bit) ones[j]++; else zeros[j]++;
        }
    }

    fprintf(stderr, "[diag] Hypercube f_i balance over %d/%d points (k=%d):\n", total, dataset->size, k);
    double avg_ones = 0.0;
    for (int j = 0; j < k; ++j)
    {
        long long tot = ones[j] + zeros[j];
        double p1 = (tot > 0) ? ((double)ones[j] / (double)tot) : 0.0;
        avg_ones += p1;
        fprintf(stderr, "  bit %2d: ones=%lld zeros=%lld p1=%.3f\n", j, ones[j], zeros[j], p1);
    }
    avg_ones /= (double)k;
    fprintf(stderr, "  avg p1 across bits = %.3f (target ~0.5)\n", avg_ones);

    free(ones); free(zeros);
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
    hyper->dataset_size = dataset->size;
    
    // Store data type and select distance function
    hyper->data_type = dataset->data_type;
    if (dataset->data_type == DATA_TYPE_FLOAT)
        hyper->distance = euclidean_distance;  // float-based distance
    else
        hyper->distance = euclidean_distance_int;  // int-based distance

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

    // Initialize 2-universal hash parameters for f functions (one a,b pair per bit)
    hyper->f_a = (uint32_t*)malloc(hyper->kproj * sizeof(uint32_t));
    hyper->f_b = (uint32_t*)malloc(hyper->kproj * sizeof(uint32_t));
    if (!hyper->f_a || !hyper->f_b)
    {
        hyper_destroy(hyper);
        exit(EXIT_FAILURE);
    }

    // Generate random a_i (must be odd for good properties) and b_i for each bit
    for (int i = 0; i < hyper->kproj; i++)
    {
        // a_i must be odd; generate random uint32 and set LSB
        hyper->f_a[i] = ((uint32_t)rand() << 16) | ((uint32_t)rand() & 0xFFFF);
        hyper->f_a[i] |= 1U; // ensure odd
        
        // b_i can be any uint32
        hyper->f_b[i] = ((uint32_t)rand() << 16) | ((uint32_t)rand() & 0xFFFF);
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
    
    // Optional diagnostic: check f_i balance if requested via env var
    const char* chk = getenv("HYPER_BALANCE");
    if (chk && chk[0] != '\0' && chk[0] != '0')
    {
        int sample = 0;
        const char* s = getenv("HYPER_BALANCE_SAMPLE");
        if (s) sample = atoi(s);
        hyper_check_f_balance(hyper, dataset, sample);
    }

    // print the contents of the hash table -- debug only
    //print_hashtable(hyper->hash_table, 1 << hyper->kproj, dataset->dimension);

    return hyper;
}

// End of patch

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
    uint64_t bucket_idx = hyper->binary_hash_function(q, hyper, &q_id);

    // Access the bucket corresponding to the computed index
    int bucket_count = 0;
    const HTEntry* bucket = hash_table_get_bucket_entries(hyper->hash_table, bucket_idx, &bucket_count);
    
    // Compute Hamming distance and probe other buckets if needed
    // Calculate binary vector representation of bucket_idx
    // Only kproj bits are needed for the cube representation
    int* bucket_vector = (int*)malloc(hyper->kproj * sizeof(int));
    for (int i = 0; i < hyper->kproj; i++)
    {
        bucket_vector[i] = (int)((bucket_idx >> (hyper->kproj - 1 - i)) & 1ULL);
    }
    
    int* neighbors = NULL;
    int neighbor_count = hyper->probes;

    // Get neighboring bucket indices based on Hamming distance
    // only the required number of probes
    int checked = 0; // number of distinct points examined
    int reached_m = 0;
    get_hamming_neighbors(bucket_vector, hyper->probes, hyper->kproj, &neighbors);
    for (int n = 0; n < neighbor_count; n++)
    {
        uint64_t neighbor_idx = 0ULL;
        // Access the n-th neighbor from the flat array
        int* current_neighbor = neighbors + (n * hyper->kproj);
        for (int b = 0; b < hyper->kproj; b++)
        {
            neighbor_idx = (neighbor_idx << 1) | (uint64_t)current_neighbor[b];
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

            // Count examined points and enforce M threshold
            checked++;
            if (checked >= hyper->M)
            {
                reached_m = 1;
                break; // exit current bucket
            }

            // Use int-based distance computation for MNIST integer data
            float dist = hyper->distance(q, p, hyper->d);

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
        if (reached_m)
            break; // exit probing further buckets
    }
    // Clean up allocations
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

    // Free 2-universal hash coefficient arrays
    if (hyper->f_a)
        free(hyper->f_a);
    if (hyper->f_b)
        free(hyper->f_b);

    // Finally free the structure itself
    free(hyper);
}
