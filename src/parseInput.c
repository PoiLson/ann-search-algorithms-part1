#include "../include/main.h"

int parse_arguments(int argc, char **argv, SearchParams *params)
{
    // Default values
    memset(params, 0, sizeof(SearchParams));
    params->seed = 1;
    params->k = 4;
    params->L = 5;
    params->w = 4.0;
    params->N = 1;
    params->R = 2000.0;
    params->dataset_type = DATA_NONE;
    params->algorithm = ALG_NONE;
    params->range_search = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            strncpy(params->dataset_path, argv[++i], sizeof(params->dataset_path));
        else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc)
            strncpy(params->query_path, argv[++i], sizeof(params->query_path));
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            strncpy(params->output_path, argv[++i], sizeof(params->output_path));
        else if (strcmp(argv[i], "-type") == 0 && i + 1 < argc)
        {
            i++;
            if (strcmp(argv[i], "mnist") == 0)
                params->dataset_type = DATA_MNIST;
            else if (strcmp(argv[i], "sift") == 0)
                params->dataset_type = DATA_SIFT;
        }
        else if (strcmp(argv[i], "-range") == 0 && i + 1 < argc)
            params->range_search = (strcmp(argv[++i], "true") == 0);
        else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc)
            params->N = atoi(argv[++i]);
        else if (strcmp(argv[i], "-R") == 0 && i + 1 < argc)
            params->R = atof(argv[++i]);

        // Algorithm-specific
        else if (strcmp(argv[i], "-lsh") == 0)
            params->algorithm = ALG_LSH;
        else if (strcmp(argv[i], "-hypercube") == 0)
            params->algorithm = ALG_HYPERCUBE;
        else if (strcmp(argv[i], "-ivfflat") == 0)
            params->algorithm = ALG_IVFFLAT;
        else if (strcmp(argv[i], "-ivfpq") == 0)
            params->algorithm = ALG_IVFPQ;

        // LSH
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc)
            params->k = atoi(argv[++i]);
        else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc)
            params->L = atoi(argv[++i]);
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc)
            params->w = atof(argv[++i]);

        // Hypercube
        else if (strcmp(argv[i], "-kproj") == 0 && i + 1 < argc)
            params->kproj = atoi(argv[++i]);
        else if (strcmp(argv[i], "-M") == 0 && i + 1 < argc)
            params->M = atoi(argv[++i]);
        else if (strcmp(argv[i], "-probes") == 0 && i + 1 < argc)
            params->probes = atoi(argv[++i]);

        // IVF / IVFPQ
        else if (strcmp(argv[i], "-kclusters") == 0 && i + 1 < argc)
            params->kclusters = atoi(argv[++i]);
        else if (strcmp(argv[i], "-nprobe") == 0 && i + 1 < argc)
            params->nprobe = atoi(argv[++i]);
        else if (strcmp(argv[i], "-seed") == 0 && i + 1 < argc)
            params->seed = atoi(argv[++i]);

        // IVFPQ only
        else if (strcmp(argv[i], "-nbits") == 0 && i + 1 < argc)
            params->nbits = atoi(argv[++i]);
        else if (strcmp(argv[i], "-Mpq") == 0 && i + 1 < argc)
            params->M_pq = atoi(argv[++i]);

        else
        {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            return -1;
        }
    }

    // Minimal checks
    if (params->algorithm == ALG_NONE || params->dataset_type == DATA_NONE)
    {
        fprintf(stderr, "Error: Missing algorithm or dataset type.\n");
        print_usage();
        return -1;
    }
    return 0;
}

// TODO
// will print the specific format of the chosen algorithm in the future
void print_usage(void)
{
    printf("Usage examples:\n");
    printf("  ./search -d data.dat -q query.dat -lsh -k 4 -L 5 -w 4.0 -N 1 -R 2000 -type mnist -range false\n");
    printf("  ./search -d data.dat -q query.dat -hypercube -kproj 14 -w 4.0 -M 10 -probes 2 -type sift\n");
    printf("  ./search -d data.dat -q query.dat -ivfflat -kclusters 50 -nprobe 5 -type mnist\n");
    printf("  ./search -d data.dat -q query.dat -ivfpq -kclusters 50 -nprobe 5 -M 16 -nbits 8 -type sift\n");
}