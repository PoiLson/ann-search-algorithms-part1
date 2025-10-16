#include "../include/main.h"

bool f(Hashmap* map, int h_ip)
{
    bool* value = hashmap_getValue(map, h_ip);
    if (value != NULL)
    {
        return *value;
    }
    else
    {
        bool bit = rand() % 2;
        hashmap_insert(map, h_ip, bit);
        return bit;
    }
}

int hash_func_impl_hyper(const void* p, const Hypercube* hyper, int ID)
{
    bool* hash_bit_array = (bool*)malloc(hyper->kproj * sizeof(bool));
    if (!hash_bit_array)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    ID = 0;
    for (int i = 0; i < hyper->kproj; i++)
    {
        float func = dot_product_float(hyper->hash_params[i].v, p, hyper->d);
        int h_i = (int)floor((func + hyper->hash_params[i].t) / hyper->w);

        hash_bit_array[i] = f(&hyper->map[i], h_i);
        ID = (ID << 1) | hash_bit_array[i]; // Shift left and add the new bit
    }


    return ID;
}

bin_hash hyper_hash(const Hypercube* hyper)
{
    return hash_func_impl_hyper;
}

int hash_function_hyper(HashTable ht, void* data, int* ID)
{
    ////aaaaaaaaa uelei global opws sto lsh???
}



Hypercube* hyper_init(const struct SearchParams* params, const struct Dataset* dataset)
{
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
    hyper->map = (Hashmap*)malloc(hyper->kproj * sizeof(Hashmap));
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
        hyper->map[i] = *hashmap_init();
    }

    // Initialize binary hash functions
    hyper->binary_hash_functions = (bin_hash*)malloc(hyper->kproj * sizeof(bin_hash));
    if (!hyper->binary_hash_functions)
    {
        for (int j = 0; j < hyper->kproj; j++)  
        {
            free(hyper->hash_params[j].v);
            hashmap_free(&hyper->map[j]);
        }
        free(hyper->map);
        free(hyper->hash_params);
        free(hyper);
        return NULL;
    }

    for (int i = 0; i < hyper->kproj; i++)
    {
        hyper->binary_hash_functions[i] = hash_function_hyper(hyper);
    }

    // Create hash table
    hyper->hash_table = hash_table_create(1 << hyper->kproj, sizeof(int), NULL, NULL, hash_function_hyper);
    if (!hyper->hash_table)
    {
        for (int j = 0; j < hyper->kproj; j++)
        {
            free(hyper->hash_params[j].v);
            hashmap_free(&hyper->map[j]);
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
        hash_table_insert(hyper->hash_table, dataset->data[i], i);
    }
    
    // print the contents of the hash table
    print_hashtable(hyper->hash_table, 1 << hyper->kproj, dataset->dimension);

    return hyper;
}

void hyper_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                        int** range_neighbors, int* range_count, void* index_data)
{
    return;
}

void hyper_destroy(struct Hypercube* hyper)
{
    return;
}
