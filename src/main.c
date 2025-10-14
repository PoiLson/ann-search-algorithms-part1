#include "../include/main.h"

int main(int argc, char **argv)
{
    SearchParams params;

    srand(time(NULL));

    if (parse_arguments(argc, argv, &params) != 0)
        return EXIT_FAILURE;

    printf("Algorithm selected: ");

    Dataset* dataset = read_data(params.dataset_path);

    if (dataset == NULL)
        return EXIT_FAILURE;

    printf("Dataset loaded: %d points of dimension %d\n", dataset->size, dataset->dimension);
    


    // //print some points
    printPartialDataset(20, dataset);


    switch (params.algorithm)
    {
        case ALG_LSH:
            printf("LSH\n");
            run_lsh(&params, dataset);
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