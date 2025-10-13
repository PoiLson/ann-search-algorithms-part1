#include "../include/parseInput.h"


// TODO all of these to be putted in another function not in main, an idea
int main(int argc, char **argv)
{
    SearchParams params;

    srand(time(NULL));

    if (parse_arguments(argc, argv, &params) != 0)
        return EXIT_FAILURE;

    printf("Algorithm selected: ");

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
