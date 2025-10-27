#include "../include/main.h"


int main()
{

    printf("hello to debugging!\n");

    float array[5][2] = {{11.5555, 2.3}, {3.78, 4.43}, {5.001, 6.1234}, {7.00008, 8.8000}, {9.0, 10.4}};
    
    fisher_yates_shuffle(array, 5, sizeof(array[0]));
    printf("%ld, %ld, %ld\n", sizeof(array), sizeof(array[0]), sizeof(float));


    for(int idx = 0; idx < 5; idx++)
    {
        printf("array[%d] = (%f, %f)\n",  idx, array[idx][0], array[idx][1]);
    }

    printf("------------------------------\n");

    float** arrayExp = (float**)malloc(sizeof(float*) * 5);
    for(int idx = 0; idx < 5; idx++)
    {
        arrayExp[idx] = (float*)malloc(sizeof(float) * 3);

        arrayExp[idx][0] = ((float)idx * 2) / 3;
        arrayExp[idx][1] = ((float)idx * 3) / 5;
        arrayExp[idx][2] = ((float)idx * 4) / 7;
    }

    for(int idx = 0; idx < 5; idx++)
    {
        printf("array[%d] = (%f, %f)\n",  idx, arrayExp[idx][0], arrayExp[idx][1]);
    }

    fisher_yates_shuffle(*arrayExp, 5, 3*sizeof(float));

    printf("AFTER FISHER\n");
    for(int idx = 0; idx < 5; idx++)
    {
        printf("array[%d] = (%f, %f)\n",  idx, arrayExp[idx][0], arrayExp[idx][1]);
    }

    printf("%ld, %ld, %ld\n", sizeof(arrayExp), sizeof(arrayExp[0]), sizeof(float));

    return 0;
}



int main2(int argc, char **argv)
{
    SearchParams params;
    // Seed RNG: allow override via HYPER_SEED env var for experiments
    const char *seed_env = getenv("HYPER_SEED");
    if (seed_env && seed_env[0] != '\0') {
        unsigned int seed = (unsigned int)atoi(seed_env);
        srand(seed);
    } else {
        // default fixed seed for reproducibility
        srand(42);
    }

    if (parse_arguments(argc, argv, &params) != 0)
        exit(EXIT_FAILURE);

    printf("Algorithm selected: ");

    Dataset* dataset = NULL;
    if (params.dataset_type == DATA_MNIST)
        dataset = read_data_mnist(params.dataset_path);
    else if(params.dataset_type == DATA_SIFT)
        dataset = read_data_sift(params.dataset_path);
    else if(params.dataset_type == DATA_EXP)
        dataset = read_data_experiment(params.dataset_path);

    if (dataset == NULL)
    {
        perror("Dataset not correct\n");
        exit(EXIT_FAILURE);
    }

    if(params.dataset_type == DATA_MNIST)
        printf("MNIST Dataset loaded: %d points of dimension %d\n", dataset->size, dataset->dimension);
    else if(params.dataset_type == DATA_SIFT)
        printf("SIFT Dataset loaded: %d points of dimension %d\n", dataset->size, dataset->dimension);
    else if(params.dataset_type == DATA_EXP)
        printf("EXPERIMENT Dataset loaded: %d points of dimension %d\n", dataset->size, dataset->dimension);

    // print some points
    // printPartialDataset(2, dataset);

    switch (params.algorithm)
    {
        case ALG_LSH:
            printf("LSH\n");
            run_lsh(&params, dataset);
            break;

        case ALG_HYPERCUBE:
            printf("Hypercube\n");
            run_hypercube(&params, dataset);
            break;

        case ALG_IVFFLAT:
            printf("IVFFlat\n");
            run_ivfflat(&params, dataset);
            break;

        case ALG_IVFPQ:
            printf("IVFPQ\n");
            run_ivfpq(&params, dataset);
            break;

        default:
            printf("Unknown\n");
            break;
    }

    // Free loaded dataset
    free_dataset(dataset);

    return EXIT_SUCCESS;
}