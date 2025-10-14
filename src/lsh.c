#include "../include/main.h"
#include "../include/parseInput.h"

struct LSH* lsh = NULL;
int table_index = 0;
extern int dimension;

// create a closure-like function that captures the LSH parameters
int hash_func_impl(const float* p ,const LSH* lsh, int table_index)
{
    int id = 0;
    for (int i = 0; i < lsh->k; i++)
    {
        int func = dot_product(lsh->hash_params[i].v, p, lsh->d);
        func = (int)floor((func + lsh->hash_params[i].t) / lsh->w);
        // combine using linear combination 
        id += func * lsh->linear_combinations[table_index][i] % lsh->num_of_buckets;
    }
    return id % lsh->num_of_buckets % lsh->table_size;
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

int compare_points(void* a, void* b)
{
    float* point_a = (float*)a;
    float* point_b = (float*)b; 
    double dist = euclidean_distance(point_a, point_b, dimension);
    return (dist == 0.0) ? 0 : (dist < 0.0) ? -1 : 1;
}
int hash_function(HashTable ht, void* key)
{
    return lsh->amplified_hash_functions[table_index]((float*)key, lsh, table_index);
}


void lsh_init(const struct SearchParams* params, int dimension, int dataset_size, float** dataset){
    printf("Running LSH with dataset: %s\n", params->dataset_path);
    lsh = (LSH*)malloc(sizeof(LSH));
    lsh->d = dimension;
    lsh->L = params->L;
    lsh->k = params->k; 
    lsh->w = params->w;
    lsh->table_size = 25; // TODO make it a parameter n/4
    lsh->num_of_buckets = 91; // TODO make it a parameter prime 
    lsh->distance = euclidean_distance;
    // till here correct

    //allocate memory for hash parameters 
    lsh->hash_params = (LSH_hash_function*)malloc(lsh->k * sizeof(LSH_hash_function));
    // generate vectors and t for each hash function
    // and store them in lsh->hash_params
    for (int i = 0; i < lsh->k; i++){
        lsh->hash_params[i].v = (float*)malloc(lsh->d * sizeof(float));
        generate_random_vector(lsh->hash_params[i].v, lsh->d);
        
        // Calculate minimum dot product for this vector across all dataset points
        float min_dot_product = 1e30f; // Large positive number
        for (int j = 0; j < dataset_size; j++) {
            float dot = dot_product(lsh->hash_params[i].v, dataset[j], lsh->d);
            if (dot < min_dot_product) {
                min_dot_product = dot;
            }
        }
        
        // Set t to ensure all hash values are positive
        // t = -min_dot_product + small_buffer to ensure positivity
        float buffer = 1.0f; // Small positive buffer
        if (min_dot_product > 0 ) lsh->hash_params[i].t = 0; // Ensure t is non-negative
        lsh->hash_params[i].t = -min_dot_product + buffer;
        
        printf("Hash function %d: min_dot_product = %f, t = %f\n", 
               i, min_dot_product, lsh->hash_params[i].t);
    }

    //print all hash parameters
    for (int i = 0; i < lsh->k; i++){
        printf("Hash function %d: v = (", i);
        for (int j = 0; j < lsh->d; j++){
            printf("%f", lsh->hash_params[i].v[j]);
            if (j < lsh->d - 1){
                printf(", ");
            }
        }
        printf("), t = %f\n", lsh->hash_params[i].t);
    } 


    //initialize linear combinations r[i][j]
    lsh->linear_combinations = (int**)malloc(lsh->L * sizeof(int*));
    for (int i = 0; i < lsh->L; i++){
        lsh->linear_combinations[i] = (int*)malloc(lsh->k * sizeof(int));
        for (int j = 0; j < lsh->k; j++){
            lsh->linear_combinations[i][j] = rand()% 10 + 1 ; // random int between 1 and 10
        }
    }

    //print all linear combinations
    for (int i = 0; i < lsh->L; i++){
        printf("Linear combination %d: (", i);
        for (int j = 0; j < lsh->k; j++){
            printf("%d", lsh->linear_combinations[i][j]);
            if (j < lsh->k - 1){
                printf(", ");
            }
        }
        printf(")\n");
    }

    //allocate memory for amplified hash functions
    lsh->amplified_hash_functions = (hash_func*)malloc(lsh->L * sizeof(hash_func));
    // generate amplified hash functions for each table
    for (int i = 0; i < lsh->L; i++){
        lsh->amplified_hash_functions[i] = amplified_hash_function(lsh, i);
    }
    
    //print results of each amplified hash function for point 0
    // for (int i = 0; i < lsh->L; i++){
    //     int hash_value = lsh->amplified_hash_functions[i](dataset[0], lsh, i);
    //     printf("Amplified hash function %d for point 0: %d\n", i, hash_value);
    // }

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));
    for (int i = 0; i < lsh->L; i++){
        lsh->hash_tables[i] = hash_table_create(lsh->table_size, NULL, compare_points, hash_function);
    }

    // insert all points in all hash tables
    for (int i = 0; i < dataset_size; i++){
        // Verify point is valid before insertion
        if (dataset[i] == NULL) {
            printf("Warning: dataset[%d] is NULL\n", i);
            continue;
        }
        
        for (int j = 0; j < lsh->L; j++){
            // printf("Inserted point %d into hash table %d", i, j);
            table_index = j; // set the global table_index for hash_function
            hash_table_insert(lsh->hash_tables[j], dataset[i], dataset[i]);
        }
    }
    
    // print the contents of each hash table
    print_hashtables(lsh, dimension);


}