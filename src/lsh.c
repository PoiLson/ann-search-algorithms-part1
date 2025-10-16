#include "../include/main.h"

struct LSH* lsh = NULL;
int table_index = 0;

// create a closure-like function that captures the LSH parameters
// Calculates ID of a vector of the dataset
// And also the hash value (g(p)) for that vector
int hash_func_impl_lsh(const void* p, const LSH* lsh, int table_index, int* ID)
{
    long long id = 0;
    for (int i = 0; i < lsh->k; i++)
    {
        float func = dot_product_float(lsh->hash_params[i].v, p, lsh->d);
        // printf("-----------------------\n");
        // printf("func: %lf\n", func);
        int h_i = (int)floor((func + lsh->hash_params[i].t) / lsh->w);
        // printf("func2: %d\n", func2);
        // printf("-----------------------\n");

        // Combine using proper linear combination: ID = sum(r_i * h_i) mod M
        long long r_i = lsh->linear_combinations[table_index][i];
        id = (id + r_i * h_i) % lsh->num_of_buckets;
        
        // Handle negative values from modulo
        if (id < 0)
            id += lsh->num_of_buckets;
    }

    *ID = (int)id;

    return (*ID) % lsh->table_size;
}


// g(p) = ID(p) mod table_size
// where ID(p) = sum(h_i(p) * r_i) mod M
// r_i are random integers saved in the LSH struct
// M is the number of buckets also saved in the LSH struct
// h_i are the hash functions saved in the LSH struct
// g() needs to be a function stored in the LSH struct so it needs to return hash_func
hash_func amplified_hash_function_lsh(const LSH* lsh, int table_index)
{
    
    return hash_func_impl_lsh;
}

int hash_function_lsh(HashTable ht, void* data, int* ID)
{
    return lsh->amplified_hash_functions[table_index](data, lsh, table_index, ID);
}


LSH* lsh_init(const struct SearchParams* params, const struct Dataset* dataset)
{
    printf("Running LSH with dataset: %s\n", params->dataset_path);
    lsh = (LSH*)malloc(sizeof(LSH));
    lsh->d = dataset->dimension;
    lsh->L = params->L;
    lsh->k = params->k; 
    lsh->w = params->w;
    // lsh->table_size = nearest_prime(dataset->size / 4); // Dynamic sizing
    // lsh->num_of_buckets = nearest_prime(lsh->table_size * 2); // Ensure prime
    lsh->table_size = 25; // TODO make it a parameter n/4, not prime necessarily
    lsh->num_of_buckets = 91; // TODO make it a parameter prime
    lsh->distance = euclidean_distance;

    // Debug Reasons
    // printf("entered lsh\n\n");
    // printPartialDataset(20, dataset);

    //allocate memory for hash parameters 
    lsh->hash_params = (LSH_hash_function*)malloc(lsh->k * sizeof(LSH_hash_function));
    if(!lsh->hash_params)
    {
        free(lsh);
        return NULL; // or exit failure?
    }

    // generate vectors and t for each hash function
    // and store them in lsh->hash_params
    for (int i = 0; i < lsh->k; i++)
    {
        lsh->hash_params[i].v = (float*)malloc(lsh->d * sizeof(float));
        generate_random_vector(lsh->hash_params[i].v, lsh->d);

        if(!lsh->hash_params[i].v)
        {
            //free memory
            return NULL; // or exit failure?
        }

        // (Debug) Print the Generated Vectors
        // for(int k = 0; k < lsh->d; k++)
        // {
        //     printf("v = %lf\n", (double)(lsh->hash_params[i].v)[k]);
        // }

        // Calculate t
        int tmp = 0;
        lsh->hash_params[i].t = uniform_distribution(&tmp, &(lsh->w));
    }



    //print all hash parameters
    // for (int i = 0; i < lsh->k; i++)
    // {
    //     printf("Hash function %d: v = (", i);

    //     for (int j = 0; j < lsh->d; j++)
    //     {
    //         printf("%f", lsh->hash_params[i].v[j]);

    //         if (j < lsh->d - 1)
    //             printf(", ");

    //     }

    //     printf("), t = %f\n", lsh->hash_params[i].t);
    // } 


    //initialize linear combinations r[i][j]
    lsh->linear_combinations = (int**)malloc(lsh->L * sizeof(int*));
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->linear_combinations[i] = (int*)malloc(lsh->k * sizeof(int));
        if(!lsh->linear_combinations[i])
        {
            //free memory
            return NULL;  // or exit failure?
        }

        for (int j = 0; j < lsh->k; j++)
        {
            lsh->linear_combinations[i][j] = rand(); // random int between 1 and 10
        }
    }

    //print all linear combinations
    // for (int i = 0; i < lsh->L; i++)
    // {
    //     printf("Linear combination %d: (", i);

    //     for (int j = 0; j < lsh->k; j++)
    //     {
    //         printf("%d", lsh->linear_combinations[i][j]);

    //         if (j < lsh->k - 1)
    //             printf(", ");
    //     }
    //     printf(")\n");
    // }

    //allocate memory for amplified hash functions
    lsh->amplified_hash_functions = (hash_func*)malloc(lsh->L * sizeof(hash_func));
    if(!lsh->amplified_hash_functions)
    {
        //free memory
        return NULL;  // or exit failure?
    }

    // generate amplified hash functions for each table
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->amplified_hash_functions[i] = amplified_hash_function_lsh(lsh, i);
    }

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->hash_tables[i] = hash_table_create(lsh->table_size, sizeof(int), NULL, compare_vectors, hash_function_lsh);
        if(!lsh->hash_tables[i])
        {
            //free memory
            
            return NULL; // or exit failure?
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
            // printf("Inserted point %d into hash table %d", i, j);
            table_index = j; // set the global table_index for hash_function
            hash_table_insert(lsh->hash_tables[j], &i, dataset->data[i]);
        }
    }
    
    // print the contents of each hash table
    print_hashtables(lsh->L, lsh->table_size, lsh->hash_tables, dataset->dimension);

    return lsh;
}

void lsh_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                     int** range_neighbors, int* range_count, void* index_data)            
{
    struct LSH* lsh = (struct LSH*)index_data;

    for (int tbl_idx = 0; tbl_idx < lsh->L; tbl_idx++)
    {
        int q_id;
        int bucket_idx = lsh->amplified_hash_functions[tbl_idx](q, lsh, tbl_idx, &q_id);

        Node bucket = hash_table_get_bucket(lsh->hash_tables[tbl_idx], bucket_idx);
        while (bucket)
        {
            int data_idx = *(int*)bucket->key;
            void* p = bucket->data;

            // if( bucket->ID != q_id )
            // {
            //     bucket = bucket->next;
            //     continue;
            // }

            // Check all points in the bucket - they're candidates
            float dist = euclidean_distance(q, p);
            printf("knn distance: %f, hashtable: %d\n", dist, tbl_idx);

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
                *range_neighbors = (int*)realloc(*range_neighbors, (*range_count + 1) * sizeof(int));

                if (*range_neighbors)
                    (*range_neighbors)[(*range_count)++] = data_idx;
                else
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    break;
                    // TODO, might need a memory cleanup if it fails
                }
            }

            bucket = bucket->next;
        }
    }
}

void lsh_destroy(struct LSH* lsh)
{
    if (lsh)
    {
        for (int i = 0; i < lsh->k; i++)
            free(lsh->hash_params[i].v);

        free(lsh->hash_params);

        for (int i = 0; i < lsh->L; i++)
        {
            free(lsh->linear_combinations[i]);
            hash_table_destroy(lsh->hash_tables[i]);
        }
        
        free(lsh->linear_combinations);
        free(lsh->amplified_hash_functions);
        free(lsh->hash_tables);
        free(lsh);
    }
}