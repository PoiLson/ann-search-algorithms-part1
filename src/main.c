#include "../include/main.h"

float** dataset = NULL;
int dataset_size = 0;
int dimension = 0;


// todo : change dataset for large dimensional data
// todo : make a struct for global variables
// todo : make a struct for hash table data containing points and their ids (to narrow down search results)
// todo : implement quering :
//              - kNN
//              - range search    





void run_lsh(SearchParams* params){
    lsh_init(params, dimension, dataset_size, dataset);
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
    // for (int i = 0; i < dataset_size; i++)
    // {
    //     printf("Point %d: ", i);
    //     for (int j = 0; j < dimension; j++)
    //     {
    //         printf("%f ", dataset[i][j]);
    //     }
    //     printf("\n");
    // }

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
