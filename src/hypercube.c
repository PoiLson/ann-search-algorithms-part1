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

static int hash_func_impl_hyper(const void* p, const Hypercube* hyper, int *ID)
{
    int id = 0;
    // Compute each h_i and map to bit using f 
    // Then combine bits into final ID i.e. concatenate bits
    for (int i = 0; i < hyper->kproj; i++)
    {
        float func = dot_product_float(hyper->hash_params[i].v, p, hyper->d);
        int h_i = (int)floor((func + hyper->hash_params[i].t) / hyper->w);

        bool bit = f(&(hyper->map[i]), h_i);
        id = (id << 1) | bit; // Shift left and add the new bit
    }

    *ID = (int)id;
    return (*ID);
}

int hash_function_hyper(HashTable ht, void* data, int* ID)
{
    //get the hypercube structure the particular hash function belongs to
    Hypercube* hyper_ctx = (Hypercube*)hash_table_get_context(ht);
    if (!hyper_ctx) 
    { 
        *ID = -1; 
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
    hyper->distance = euclidean_distance;

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
        hyper->map[i] = hashmap_init();
        if (!hyper->map[i])
        {
            hyper_destroy(hyper);
            exit(EXIT_FAILURE);
        }
    }

    // Set the single binary hash function for hypercube
    // (computes all k bits to form the bucket index)
    hyper->binary_hash_function = hash_func_impl_hyper;

    // Create hash table with context and index 0
    hyper->hash_table = hash_table_create(1 << hyper->kproj, sizeof(int), NULL, NULL, hash_function_hyper, hyper, 0);
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

    // Compute the bucket index for the query point
    int q_id;
    int bucket_idx = hyper->binary_hash_function(q, hyper, &q_id);

    // Access the bucket corresponding to the computed index
    Node bucket = hash_table_get_bucket(hyper->hash_table, bucket_idx);
    
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
        Node neighbor_bucket = hash_table_get_bucket(hyper->hash_table, neighbor_idx);

        // Process the neighbor bucket
        while (neighbor_bucket)
        {
            int data_idx = *(int*)neighbor_bucket->key;
            void* p = neighbor_bucket->data;

            // Check all points in the bucket - they're candidates
            float dist = euclidean_distance(q, p);

            // Check if this point is already in the result set (avoid duplicates)
            int already_found = 0;
            for (int i = 0; i < *approx_count; i++)
            {
                if (approx_neighbors[i] == data_idx)
                {
                    already_found = 1;
                    break;
                }
            }

            if (already_found)
            {
                neighbor_bucket = neighbor_bucket->next;
                continue;
            }

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

            // Range search with deduplication
            if (params->range_search && dist <= params->R)
            {
                // Check if this point is already in range_neighbors (avoid duplicates)
                int already_in_range = 0;
                for (int r = 0; r < *range_count; r++)
                {
                    if ((*range_neighbors)[r] == data_idx)
                    {
                        already_in_range = 1;
                        break;
                    }
                }
                
                if (!already_in_range)
                {
                    *range_neighbors = (int*)realloc(*range_neighbors, (*range_count + 1) * sizeof(int));

                    if (*range_neighbors)
                        (*range_neighbors)[(*range_count)++] = data_idx;
                    else
                    {
                        fprintf(stderr, "Memory reallocation failed\n");
                        //memory cleanup
                        free(*range_neighbors);
                        *range_neighbors = NULL;
                        *range_count = 0;
                        break;
                    }
                }
            }

            neighbor_bucket = neighbor_bucket->next;
        }
    }

    // Clean up
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
