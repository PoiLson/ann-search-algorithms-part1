#include "../include/main.h"


int main2()
{

    printf("hello to debugging!\n");
    int probes = 6;
    int kproj = 14;
    uint64_t bucket = 1027;
    uint64_t* neighbors = (uint64_t*)malloc(probes * sizeof(uint64_t));

    get_hamming_neighbors(bucket, probes, kproj, neighbors);

    for(int idx = 0; idx < probes; idx++)
    {
        printf("Neighbor %d: %ld\n", idx, neighbors[idx]);
    }

    return 0;
}



int main(int argc, char **argv)
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