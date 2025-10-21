#include "../include/main.h"

// Calculates ID of a vector of the dataset
// And also the hash value (g(p)) for that vector
int hash_func_impl_lsh(const void* p, const LSH* lsh, int table_index, int* ID)
{
    printf("LSH: table %d, id=%d, bucket=%d\n", table_index, *ID, (*ID) % lsh->table_size);

    long long id = 0;
    for (int i = 0; i < lsh->k; i++)
    {
        float func = dot_product_float(lsh->hash_params[i].v, p, lsh->d);
        int h_i = (int)floor((func + lsh->hash_params[i].t) / lsh->w);

        // Combine using linear combination: ID = sum(r_i * h_i) mod M
        long long r_i = lsh->linear_combinations[table_index][i];
        id = (id + r_i * h_i) % lsh->num_of_buckets;
        
        // Handle negative values from modulo
        if (id < 0)
            id += lsh->num_of_buckets;
    }

    *ID = (int)id;
    return (*ID) % lsh->table_size;
}


int hash_function_lsh(HashTable ht, void* data, int* ID)
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

    return lsh_ctx->amplified_hash_functions[t_idx](data, lsh_ctx, t_idx, ID);
    // ERROR: amplified functions is just an array of function pointers that
    // all currently point to hash_func_impl_lsh
    // dow e need a separate array if all are indentical (since the table index is a parameter)
    // or do we keep it for later extensions?
}


LSH* lsh_init(const struct SearchParams* params, const struct Dataset* dataset)
{
    // Allocate memory for LSH structure and set parameters
    LSH* lsh = (LSH*)malloc(sizeof(LSH));
    lsh->d = dataset->dimension;
    lsh->L = params->L;
    lsh->k = params->k; 
    lsh->w = params->w;
    // lsh->table_size = dataset->size / 4; 
    // lsh->num_of_buckets = 1 << 32 - 5; // max prime for 32-bit int -> ERROR: calculates 1 << 27, if want max prime then we have to assign it as: 4294967291u
    
    lsh->table_size = nearest_prime(dataset->size / lsh->L); // e.g., ~3000
    lsh->num_of_buckets = nearest_prime(lsh->table_size * 2); // e.g., ~6000

    lsh->distance = euclidean_distance;

    //allocate memory for hash parameters 
    lsh->hash_params = (LSH_hash_function*)malloc(lsh->k * sizeof(LSH_hash_function));
    if(!lsh->hash_params)
    {
        lsh_destroy(lsh);
        exit(EXIT_FAILURE);
    }

    // generate vectors and t for each hash function
    // and store them in lsh->hash_params
    for (int i = 0; i < lsh->k; i++)
    {
        lsh->hash_params[i].v = (float*)malloc(lsh->d * sizeof(float));
        generate_random_vector(lsh->hash_params[i].v, lsh->d);

        if(!lsh->hash_params[i].v)
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }

    // Calculate t uniformly in [0, w)
    float zero = 0.0f;
    float wval = lsh->w;
    lsh->hash_params[i].t = uniform_distribution(&zero, &wval);
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
            lsh->linear_combinations[i][j] = rand(); // random int for r[i][j]
        }
    }

    //allocate memory for amplified hash functions
    lsh->amplified_hash_functions = (hash_func*)malloc(lsh->L * sizeof(hash_func));
    if(!lsh->amplified_hash_functions)
    {
        lsh_destroy(lsh);
        exit(EXIT_FAILURE);
    }

    // generate amplified hash functions for each table
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->amplified_hash_functions[i] = hash_func_impl_lsh;
    }

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));
    // ERROR WANT TO CHECK STH
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

    // For each hash table, compute the bucket index for the query point
    for (int tbl_idx = 0; tbl_idx < lsh->L; tbl_idx++)
    {
        int q_id;
        int bucket_idx = lsh->amplified_hash_functions[tbl_idx](q, lsh, tbl_idx, &q_id);

        Node bucket = hash_table_get_bucket(lsh->hash_tables[tbl_idx], bucket_idx);
        while (bucket)
        {
            int data_idx = *(int*)bucket->key;
            void* p = bucket->data;

            // Check all points in the bucket - they're candidates
            float dist = euclidean_distance(q, p, lsh->d);

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
                bucket = bucket->next;
                continue;
            }

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
            bucket = bucket->next;
        }
    }

    //cleanup if no range neighbors found
    if (*range_count == 0)
    {
        free(*range_neighbors);
        *range_neighbors = NULL;
    }
}

void lsh_destroy(struct LSH* lsh)
{
    if (!lsh)
        return;

    // Free hash params and their vectors
    if (lsh->hash_params)
    {
        for (int i = 0; i < lsh->k; i++)
        {
            if (lsh->hash_params[i].v)
                free(lsh->hash_params[i].v);
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

    if (lsh->amplified_hash_functions)
        free(lsh->amplified_hash_functions);

    free(lsh);
}