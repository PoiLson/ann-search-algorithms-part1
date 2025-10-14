#include "../include/main.h"


void run_lsh(SearchParams* params, Dataset* dataset)
{
    // Placeholder for LSH algorithm implementation
    lsh_init(params, dataset);
    return;
}

void run_hypercube(SearchParams* params)
{
    // Placeholder for Hypercube algorithm implementation
    printf("Running Hypercube with dataset: %s\n", params->dataset_path);
    return;
}

void run_ivfflat(SearchParams* params)
{
    // Placeholder for IVFFlat algorithm implementation
    printf("Running IVFFlat with dataset: %s\n", params->dataset_path);
    return;
}

void run_ivfpq(SearchParams* params)
{
    // Placeholder for IVFPQ algorithm implementation
    printf("Running IVFPQ with dataset: %s\n", params->dataset_path);
    return;
}
