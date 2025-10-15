#include "../include/main.h"

struct LSH* lsh = NULL;
int table_index = 0;

// create a closure-like function that captures the LSH parameters
// Calculates ID of a vector of the dataset
// And also the hash value (g(p)) for that vector
int hash_func_impl(const void* p, const LSH* lsh, int table_index, int* ID)
{
    int id = 0;
    for (int i = 0; i < lsh->k; i++)
    {
        float func = dot_product_float(lsh->hash_params[i].v, p, lsh->d);
        // printf("-----------------------\n");
        // printf("func: %lf\n", func);
        int func2 = (int)floor((func + lsh->hash_params[i].t) / lsh->w);
        // printf("func2: %d\n", func2);
        // printf("-----------------------\n");

        // Combine using linear combination 
        int x = (func2 % lsh->num_of_buckets) + lsh->num_of_buckets;
        int y = (lsh->linear_combinations[table_index][i] % lsh->num_of_buckets) + lsh->num_of_buckets;

        id += ((x % lsh->num_of_buckets) * (y % lsh->num_of_buckets)) % lsh->num_of_buckets;
    }

    *ID = id % lsh->num_of_buckets;

    return (*ID) % lsh->table_size;
}


// g(p) = ID(p) mod table_size
// where ID(p) = sum(h_i(p) * r_i) mod M
// r_i are random integers saved in the LSH struct
// M is the number of buckets also saved in the LSH struct
// h_i are the hash functions saved in the LSH struct
// g() needs to be a function stored in the LSH struct so it needs to return hash_func
hash_func amplified_hash_function(const LSH* lsh, int table_index)
{
    
    return hash_func_impl;
}

int compare_vectors(void* a, void* b)
{
    printf("should be correct!\t\t");
    float dist = euclidean_distance(a, b);
    return (dist == 0.0) ? 0 : (dist < 0.0) ? -1 : 1;
}

int hash_function(HashTable ht, void* data, int* ID)
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
    for (int i = 0; i < lsh->k; i++)
    {
        printf("Hash function %d: v = (", i);

        for (int j = 0; j < lsh->d; j++)
        {
            printf("%f", lsh->hash_params[i].v[j]);

            if (j < lsh->d - 1)
                printf(", ");

        }

        printf("), t = %f\n", lsh->hash_params[i].t);
    } 


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
    for (int i = 0; i < lsh->L; i++)
    {
        printf("Linear combination %d: (", i);

        for (int j = 0; j < lsh->k; j++)
        {
            printf("%d", lsh->linear_combinations[i][j]);

            if (j < lsh->k - 1)
                printf(", ");
        }
        printf(")\n");
    }

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
        lsh->amplified_hash_functions[i] = amplified_hash_function(lsh, i);
    }

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));
    for (int i = 0; i < lsh->L; i++)
    {
        lsh->hash_tables[i] = hash_table_create(lsh->table_size, sizeof(int), NULL, compare_vectors, hash_function);
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