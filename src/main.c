#include "../include/main.h"

float** dataset = NULL;
int dataset_size = 0;
int dimension = 0;
struct LSH* lsh = NULL;
int table_index = 0;
// todo : 
// todo : make a struct for global variables
// todo : make a struct for hash table data containing points and their ids (to narrow down search results)
// todo : implement quering :
//              - kNN
//              - range search    

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

void run_lsh(SearchParams* params){
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
    for (int i = 0; i < lsh->L; i++){
        int hash_value = lsh->amplified_hash_functions[i](dataset[0], lsh, i);
        printf("Amplified hash function %d for point 0: %d\n", i, hash_value);
    }

    // create a hash table for each hash table in LSH
    HashTable* hash_tables = (HashTable*)malloc(lsh->L * sizeof(HashTable));
    for (int i = 0; i < lsh->L; i++){
        hash_tables[i] = hash_table_create(lsh->table_size, NULL, compare_points, hash_function);
    }

    // insert all points in all hash tables
    for (int i = 0; i < dataset_size; i++){
        // Verify point is valid before insertion
        if (dataset[i] == NULL) {
            printf("Warning: dataset[%d] is NULL\n", i);
            continue;
        }
        
        for (int j = 0; j < lsh->L; j++){
            printf("Inserted point %d into hash table %d", i, j);
            table_index = j; // set the global table_index for hash_function
            hash_table_insert(hash_tables[j], dataset[i], dataset[i]);
        }
    }
    // print the contents of each hash table
    for (int i = 0; i < lsh->L; i++){
        printf("Hash table %d:\n", i);
        for (int j = 0; j < lsh->table_size; j++){
            Node bucket = hash_table_get_bucket(hash_tables[i], j);
            if (bucket != NULL){
                printf(" Bucket %d: ", j);
                Node current = bucket;
                while (current != NULL ){
                    // Add comprehensive null checks for safety
                    if (current->data == NULL) {
                        printf("(NULL_DATA) -> ");
                    } else {
                        // Verify that the data pointer is accessible
                        float* point = (float*)current->data;
                        // Check if we can safely read the point
                        printf("(");
                        for (int d = 0; d < dimension; d++){
                            printf("%f", point[d]);
                            if (d < dimension - 1){
                                printf(", ");
                            }
                        }
                        printf(") -> ");
                    }
                    
                    // Check if next pointer is accessible before dereferencing
                    if (current->next != NULL) {
                        current = current->next;
                    } else {
                        current = NULL;
                    }
                }
                printf("NULL\n");
            }
            else{
                printf(" Bucket %d: NULL\n", j);
            }
        }
        printf("----------------------------------------------------------------\n");
    }



}




void run_hypercube(SearchParams* params){
    // Placeholder for Hypercube algorithm implementation
    printf("Running Hypercube with dataset: %s\n", params->dataset_path);
}
void run_ivfflat(SearchParams* params){
    // Placeholder for IVFFlat algorithm implementation
    printf("Running IVFFlat with dataset: %s\n", params->dataset_path);
}
void run_ivfpq(SearchParams* params){
    // Placeholder for IVFPQ algorithm implementation
    printf("Running IVFPQ with dataset: %s\n", params->dataset_path);
}


float** read_data(const char* dataset_path){
    //opens a file with first line the number of points and the dimension
    FILE* file = fopen(dataset_path, "r");
    if (file == NULL){
        fprintf(stderr, "Error opening file: %s\n", dataset_path);
        return NULL;
    }
    
    // Read dataset size and dimension
    if (fscanf(file, "%d %d", &dataset_size, &dimension) != 2) {
        fprintf(stderr, "Error reading dataset size and dimension\n");
        fclose(file);
        return NULL;
    }
    
    dataset = (float**)malloc(dataset_size * sizeof(float*));
    if (dataset == NULL) {
        fprintf(stderr, "Error allocating memory for dataset\n");
        fclose(file);
        return NULL;
    }
    
    for (int i = 0; i < dataset_size; i++){
        dataset[i] = (float*)malloc(dimension * sizeof(float));
        if (dataset[i] == NULL) {
            fprintf(stderr, "Error allocating memory for point %d\n", i);
            fclose(file);
            return NULL;
        }
        
        for (int j = 0; j < dimension; j++){
            if (fscanf(file, "%f", &dataset[i][j]) != 1) {
                fprintf(stderr, "Error reading point %d, coordinate %d\n", i, j);
                fclose(file);
                return NULL;
            }
        }
    }
    fclose(file);
    return dataset;
}

// TODO all of these to be putted in another function not in main, an idea
int main(int argc, char **argv)
{
    SearchParams params;

    srand(time(NULL));

    if (parse_arguments(argc, argv, &params) != 0)
        return EXIT_FAILURE;

    printf("Algorithm selected: ");

    dataset = read_data(params.dataset_path);
    if (dataset == NULL)
        return EXIT_FAILURE;
    printf("Dataset loaded: %d points of dimension %d\n", dataset_size, dimension);
    //print all points
    for (int i = 0; i < dataset_size; i++)
    {
        printf("Point %d: ", i);
        for (int j = 0; j < dimension; j++)
        {
            printf("%f ", dataset[i][j]);
        }
        printf("\n");
    }

    switch (params.algorithm)
    {
        case ALG_LSH:
            printf("LSH\n");
            run_lsh(&params);
            break;

        case ALG_HYPERCUBE:
            printf("Hypercube\n");
            run_hypercube(&params);
            break;

        case ALG_IVFFLAT:
            printf("IVFFlat\n");
            run_ivfflat(&params);
            break;

        case ALG_IVFPQ:
            printf("IVFPQ\n");
            run_ivfpq(&params);
            break;

        default:
            printf("Unknown\n");
            break;
    }

    return EXIT_SUCCESS;
}
