#include "../include/main.h"

// Calculates ID of a vector of the dataset
// And also the hash value (g(p)) for that vector
int hash_func_impl_lsh(const void* p, const LSH* lsh, int table_index, uint64_t* outID)
{
    // printf("LSH: table %d, id=%d, bucket=%d\n", table_index, *ID, (*ID) % lsh->table_size);

    // Work modulo M using 64-bit unsigned where M may be up to 2^32-5
    uint64_t M = lsh->num_of_buckets;
    uint64_t id = 0;
    // Use the hash parameters corresponding to this table
    const LSH_hash_function* table_hash_params = lsh->hash_params[table_index];
    for (int i = 0; i < lsh->k; i++)
    {
        float func = dot_product_float_int(table_hash_params[i].v, p, lsh->d);
        // printf("  LSH: dot product(%d) = %.2f\n", i, func);
        
        float val = (func + table_hash_params[i].t) / lsh->w;
        
        // Fast integer truncation instead of floor()
        // For negative values, (int) truncates toward zero, but floor goes toward -infinity
        int h_i = (int)val;
        if (val < 0.0f && val != (float)h_i)
            h_i--;

        // Combine using linear combination: ID = sum(r_i * h_i) mod M
        long long r_i = lsh->linear_combinations[table_index][i];
        long long prod = (long long)r_i * (long long)h_i; // may be negative
        long long modM = (M == 0) ? 0 : (prod % (long long)M);
        if (modM < 0)
            modM += (long long)M;

        id = (id + (uint64_t)modM) % M;
    }

    if (outID)
        *outID = id; // expose full 64-bit ID
    int bucket_idx = (int)(id % (uint64_t)lsh->table_size);
    // printf("LSH: table %d, id=%d, bucket=%d\n", table_index, *ID, bucket_idx);

    return bucket_idx;
}


int hash_function_lsh(HashTable ht, void* data, uint64_t* ID)
{
    // Get LSH structure from hash table algorithm context
    // also get the table index to know which amplified hash function to use
    LSH* lsh_ctx = (LSH*)hash_table_get_algorithm_context(ht);
    if (!lsh_ctx)
    {
        *ID = -1;
        return 0;
    }

    int t_idx = hash_table_get_index(ht);

    return hash_func_impl_lsh(data, lsh_ctx, t_idx, ID);
}


LSH* lsh_init(const struct SearchParams* params, const struct Dataset* dataset)
{
    // Allocate memory for LSH structure and set parameters
    LSH* lsh = (LSH*)malloc(sizeof(LSH));
    lsh->d = dataset->dimension;
    lsh->L = params->L;
    lsh->k = params->k; 
    lsh->w = params->w;
    lsh->dataset_size = dataset->size;
    // lsh->table_size = dataset->size / 4; 
    // lsh->num_of_buckets = (1 << 16) - 4; // need a larger number?

    lsh->table_size = nearest_prime(dataset->size / 4);
    // Set M to the largest 32-bit prime (2^32 - 5) to avoid overflow in older impls
    lsh->num_of_buckets = 4294967291ULL;

    lsh->distance = euclidean_distance_int;

    // Allocate memory for per-table hash parameters (L x k)
    lsh->hash_params = (LSH_hash_function**)malloc(lsh->L * sizeof(LSH_hash_function*));
    if (!lsh->hash_params)
    {
        lsh_destroy(lsh);
        exit(EXIT_FAILURE);
    }

    // For each table, generate K independent hash functions (v, t)
    for (int tbl = 0; tbl < lsh->L; tbl++)
    {
        lsh->hash_params[tbl] = (LSH_hash_function*)malloc(lsh->k * sizeof(LSH_hash_function));
        if (!lsh->hash_params[tbl])
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < lsh->k; i++)
        {
            lsh->hash_params[tbl][i].v = (float*)malloc(lsh->d * sizeof(float));
            if (!lsh->hash_params[tbl][i].v)
            {
                lsh_destroy(lsh);
                exit(EXIT_FAILURE);
            }
            generate_random_vector(lsh->hash_params[tbl][i].v, lsh->d);
            // Optional: Normalize projection vector for cosine LSH; keep commented for E2LSH behavior
            // normalize_vector(lsh->hash_params[tbl][i].v, lsh->d);

            // Calculate t uniformly in [0, w)
            float zero = 0.0f;
            float wval = lsh->w;
            lsh->hash_params[tbl][i].t = uniform_distribution(&zero, &wval);
        }
    }

    //initialize linear combinations r[i][j]
    lsh->linear_combinations = (int**)malloc(lsh->L * sizeof(int*));
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->linear_combinations[i] = (int*)malloc(lsh->k * sizeof(int));
        if(!lsh->linear_combinations[i])
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < lsh->k; j++)
        {
            int tmp = rand();
            while (tmp == 0) {
                tmp = rand(); // ensure non-zero
            }
            // bound them in range [1, 1000]
            tmp = (tmp % 1000) + 1;
            lsh->linear_combinations[i][j] = tmp; // random int for r[i][j]
        }
    }

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));

    for (int i = 0; i < lsh->L; i++)
    {
        lsh->hash_tables[i] = hash_table_create(lsh->table_size, sizeof(int), NULL, compare_vectors, hash_function_lsh, lsh, i, &(dataset->dimension));
        
        if(!lsh->hash_tables[i])
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }
    }

    // insert all points in all hash tables
    for (int i = 0; i < dataset->size; i++)
    {
        // Verify point is valid before insertion
        if (!dataset->data || !dataset->data[i])
        {
            printf("Warning: dataset[%d] is NULL\n", i);
            continue;
        }
        
        for (int j = 0; j < lsh->L; j++)
        {
            // printf("INSERTING INSIDE THE HASHTABLE LSH, i = %d\n", i);
            hash_table_insert(lsh->hash_tables[j], &i, dataset->data[i]);
        }
    }

    // print the contents of each hash table -- debug only
    //print_hashtables(lsh->L, lsh->table_size, lsh->hash_tables, dataset->dimension);

    return lsh;
}

void lsh_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                     int** range_neighbors, int* range_count, void* index_data)            
{
    // Cast index_data to LSH structure
    struct LSH* lsh = (struct LSH*)index_data;

    // Use a visited array for O(1) duplicate detection across all tables
    bool* visited = (bool*)calloc(lsh->dataset_size, sizeof(bool));
    if (!visited)
    {
        fprintf(stderr, "Failed to allocate visited array.\n");
        return;
    }

    // Range neighbors dynamic capacity (grow in chunks instead of per-item realloc)
    const int RANGE_ALLOC_CHUNK = 128;
    int range_capacity = (*range_neighbors && *range_count > 0) ? *range_count : 0;

    // Debug/diagnostic: track how many UNIQUE candidates we examine across all L tables
    // We keep a simple dynamic array of seen indices for this query (linear check is fine for diagnostics)
    // int* seen_candidates = NULL;
    // int seen_count = 0;

    // Early stopping: prevent examining excessive candidates (protects against degenerate cases)
    // const int MAX_CANDIDATES = 25 * lsh->L;  // Configurable threshold (e.g., 10L-25L)

    // For each hash table, compute the bucket index for the query point
    for (int tbl_idx = 0; tbl_idx < lsh->L; tbl_idx++)
    {
        uint64_t q_id = 0ULL;
        // printf("INSIDE OF LSH INDEX LOOKUP\n");
        int bucket_idx = hash_func_impl_lsh(q, lsh, tbl_idx, &q_id);

        int bucket_count = 0;
        const HTEntry* bucket = hash_table_get_bucket_entries(lsh->hash_tables[tbl_idx], bucket_idx, &bucket_count);
        for (int bi = 0; bi < bucket_count; ++bi)
        {
            int data_idx = *(int*)bucket[bi].key;
            void* p = bucket[bi].data;




            if (bucket[bi].ID != q_id)
                continue;




            // Count unique candidates (union of buckets across all tables)
            // int already_seen = 0;
            // for (int s = 0; s < seen_count; s++)
            // {
            //     if (seen_candidates[s] == data_idx)
            //     {
            //         already_seen = 1;
            //         break;
            //     }
            // }
            // if (!already_seen)
            // {
            //     int* tmp = (int*)realloc(seen_candidates, (seen_count + 1) * sizeof(int));
            //     if (tmp)
            //     {
            //         seen_candidates = tmp;
            //         seen_candidates[seen_count++] = data_idx;
            //     }
            //     else
            //     {
            //         // If realloc fails, skip counting further to avoid crashing diagnostics
            //         // (algorithm continues functioning without candidate stats)
            //     }
            // }

            // Skip duplicate candidates across tables using visited array
            if (data_idx >= 0 && data_idx < lsh->dataset_size && visited[data_idx])
                continue;

            // Mark as visited now to avoid reprocessing
            if (data_idx >= 0 && data_idx < lsh->dataset_size)
                visited[data_idx] = true;

            // Compute distance for this candidate
            float dist = euclidean_distance_int(q, p, lsh->d);

            if (*approx_count < params->N || dist < approx_dists[*approx_count - 1])
            {
                if (*approx_count < params->N)
                    (*approx_count)++;


                int i = 0;
                for (i = *approx_count - 1; i > 0 && dist < approx_dists[i - 1]; i--)
                {
                    approx_neighbors[i] = approx_neighbors[i - 1];
                    approx_dists[i] = approx_dists[i - 1];
                }
                
                approx_neighbors[i] = data_idx;
                approx_dists[i] = dist;
            }

            if (params->range_search && dist <= params->R)
            {
                // Ensure capacity and append without per-item realloc
                if (*range_count >= range_capacity)
                {
                    int new_capacity = range_capacity + RANGE_ALLOC_CHUNK;
                    int* new_buf = (int*)realloc(*range_neighbors, new_capacity * sizeof(int));
                    if (!new_buf)
                    {
                        fprintf(stderr, "Memory reallocation failed for range neighbors.\n");
                        free(*range_neighbors);
                        *range_neighbors = NULL;
                        *range_count = 0;
                        break;
                    }
                    *range_neighbors = new_buf;
                    range_capacity = new_capacity;
                }

                // Append this neighbor
                (*range_neighbors)[(*range_count)++] = data_idx;
            }
        }
    }

    // Print diagnostic: total unique candidates examined for this query
    // Rule of thumb: if consistently < 10*L, not enough overlap/collisions
    // printf("Candidates examined (unique): %d (L=%d, threshold ~%d)\n", seen_count, lsh->L, 10 * lsh->L);

    // Free diagnostic buffer
    // free(seen_candidates);

    // cleanup if no range neighbors found
    if (*range_count == 0)
    {
        free(*range_neighbors);
        *range_neighbors = NULL;
    }

    free(visited);
}

void lsh_destroy(struct LSH* lsh)
{
    if (!lsh)
        return;

    // Free hash params and their vectors
    if (lsh->hash_params)
    {
        for (int tbl = 0; tbl < lsh->L; tbl++)
        {
            if (lsh->hash_params[tbl])
            {
                for (int i = 0; i < lsh->k; i++)
                {
                    if (lsh->hash_params[tbl][i].v)
                        free(lsh->hash_params[tbl][i].v);
                }
                free(lsh->hash_params[tbl]);
            }
        }
        free(lsh->hash_params);
    }

    // Free linear combinations
    if (lsh->linear_combinations)
    {
        for (int i = 0; i < lsh->L; i++)
        {
            if (lsh->linear_combinations[i])
                free(lsh->linear_combinations[i]);
        }
        free(lsh->linear_combinations);
    }

    // Destroy hash tables (guard duplicates) and free array
    if (lsh->hash_tables)
    {
        for (int i = 0; i < lsh->L; i++)
        {
            if (lsh->hash_tables[i])
            {
                int duplicate = 0;
                for (int j = 0; j < i; j++)
                {
                    if (lsh->hash_tables[j] == lsh->hash_tables[i])
                    {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate)
                {
                    hash_table_destroy(lsh->hash_tables[i]);
                }
                lsh->hash_tables[i] = NULL;
            }
        }
        free(lsh->hash_tables);
    }

    free(lsh);
}