#include "../include/main.h"

struct Hypercube* hyper = NULL;
int hyper_table_index = 0;

bool f(Hashmap** map, int h_ip)
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

int hash_func_impl_hyper(const void* p, const Hypercube* hyper, int *ID)
{
    int id = 0;
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

bin_hash hyper_hash(const Hypercube* hyper, int index)
{
    return hash_func_impl_hyper;
}

int hash_function_hyper(HashTable ht, void* data, int* ID)
{
   return hyper->binary_hash_functions[hyper_table_index](data, hyper, ID);
}



Hypercube* hyper_init(const struct SearchParams* params, const struct Dataset* dataset)
{
    hyper = (Hypercube*)malloc(sizeof(Hypercube));
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
        free(hyper);
        return NULL;
    }

    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->hash_params[i].v = (float*)malloc(hyper->d * sizeof(float));
        generate_random_vector(hyper->hash_params[i].v, hyper->d);

        if(!hyper->hash_params[i].v)
        {
            for (int j = 0; j < i; j++)
            {
                free(hyper->hash_params[j].v);
            }
            free(hyper->hash_params);
            free(hyper);
            exit(EXIT_FAILURE);
        }

        int tmp = 0;
        hyper->hash_params[i].t = uniform_distribution(&tmp, &(hyper->w));
    }

    // Initialize hashmaps for f functions
    hyper->map = (Hashmap**)malloc(hyper->kproj * sizeof(Hashmap*));
    if (!hyper->map)
    {
        for (int j = 0; j < hyper->kproj; j++)
        {
            free(hyper->hash_params[j].v);
        }
        free(hyper->hash_params);
        free(hyper);
        return NULL;
    }

    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->map[i] = hashmap_init();
        if (!hyper->map[i])
        {
            // Free previously allocated hashmaps
            for (int j = 0; j < i; j++)
            {
                hashmap_free(hyper->map[j]);
            }
            for (int j = 0; j < hyper->kproj; j++)
            {
                free(hyper->hash_params[j].v);
            }
            free(hyper->map);
            free(hyper->hash_params);
            free(hyper);
            return NULL;
        }
    }

    // Initialize binary hash functions
    hyper->binary_hash_functions = (bin_hash*)malloc(hyper->kproj * sizeof(bin_hash));
    if (!hyper->binary_hash_functions)
    {
        for (int j = 0; j < hyper->kproj; j++)  
        {
            free(hyper->hash_params[j].v);
            hashmap_free(hyper->map[j]);
        }
        free(hyper->map);
        free(hyper->hash_params);
        free(hyper);
        return NULL;
    }

    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->binary_hash_functions[i] = hyper_hash(hyper, i);
    }

    // Create hash table
    hyper->hash_table = hash_table_create(1 << hyper->kproj, sizeof(int), NULL, NULL, hash_function_hyper);
    if (!hyper->hash_table)
    {
        for (int j = 0; j < hyper->kproj; j++)
        {
            free(hyper->hash_params[j].v);
            hashmap_free(hyper->map[j]);
        }
        free(hyper->binary_hash_functions);
        free(hyper->map);
        free(hyper->hash_params);
        free(hyper);
        return NULL;
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
    
    // print the contents of the hash table
    print_hashtable(hyper->hash_table, 1 << hyper->kproj, dataset->dimension);

    return hyper;
}

void hyper_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                        int** range_neighbors, int* range_count, void* index_data)
{
    struct Hypercube* hyper = (struct Hypercube*)index_data;

    int q_id;
    int bucket_idx = hyper->binary_hash_functions[0](q, hyper, &q_id);


    Node bucket = hash_table_get_bucket(hyper->hash_table, bucket_idx);
    //compute hamming distance and probe other buckets if needed
    //calculate binary vector representation of bucket_idx
    int* bucket_vector = (int*)malloc(hyper->d * sizeof(int));
    printf("hyper dimension: %d\n", hyper->kproj);
    printf("vector of bucket %d: ", bucket_idx);
    for (int i = 0; i < hyper->kproj; i++)
    {
        bucket_vector[i] = (bucket_idx >> (hyper->kproj - 1 - i)) & 1;
        printf("%d", bucket_vector[i]);
    }
    printf("\n");
    int* neighbors = NULL;
    int neighbor_count = hyper->probes;

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
        printf("Probing neighbor bucket index: %d\n", neighbor_idx);
        Node neighbor_bucket = hash_table_get_bucket(hyper->hash_table, neighbor_idx);

        // Process the neighbor bucket
        while (neighbor_bucket)
        {
            int data_idx = *(int*)neighbor_bucket->key;
            void* p = neighbor_bucket->data;

            // Check all points in the bucket - they're candidates
            float dist = euclidean_distance(q, p);
            printf("knn distance: %f, hashtable: %d\n", dist, neighbor_idx);

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
            neighbor_bucket = neighbor_bucket->next;
        }
    }
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

    // Free binary hash functions array
    if (hyper->binary_hash_functions)
        free(hyper->binary_hash_functions);

    // Finally free the structure itself
    free(hyper);
}
