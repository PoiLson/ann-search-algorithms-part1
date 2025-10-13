#include "../include/parseInput.h"


int main(int argc, char **argv)
{
    SearchParams params;

    if (parse_arguments(argc, argv, &params) != 0)
        return EXIT_FAILURE;

    printf("Algorithm selected: ");

    switch (params.algorithm)
    {
        case ALG_LSH:
            printf("LSH\n");
            break;

        case ALG_HYPERCUBE:
            printf("Hypercube\n");
            break;

        case ALG_IVFFLAT:
            printf("IVFFlat\n");
            break;

        case ALG_IVFPQ:
            printf("IVFPQ\n");
            break;

        default:
            printf("Unknown\n");
            break;

    }

    /*
    switch (params.algorithm) {
        case ALG_LSH:
            run_lsh(&params);
            break;

        case ALG_HYPERCUBE:
            run_hypercube(&params);
            break;

        case ALG_IVFFLAT:
            run_ivfflat(&params);
            break;

        case ALG_IVFPQ:
            run_ivfpq(&params);
            break;
    }
    */

    return EXIT_SUCCESS;
}
