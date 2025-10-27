#include "../include/main.h"

void run_lsh(SearchParams* params, Dataset* dataset)
{
    struct LSH* lsh = lsh_init(params, dataset);
    printf("exiting...\n");
    if (!lsh) return;
    
    Dataset* query_set = NULL;
    if (params->dataset_type == DATA_MNIST)
        query_set = read_data_mnist(params->query_path);
    else if (params->dataset_type == DATA_SIFT)
        query_set = read_data_sift(params->query_path);
    else
        query_set = read_data_experiment(params->query_path);

    if (query_set)
    {
        perform_query(params, dataset, query_set, lsh_index_lookup, lsh);

        for (int i = 0; i < query_set->size; i++)
            free(query_set->data[i]);

        free(query_set->data);
        free(query_set);
    }

    lsh_destroy(lsh);

    return;
}

void run_hypercube(SearchParams* params, Dataset* dataset)
{
    struct Hypercube* hyper = hyper_init(params, dataset);

    Dataset* query_set = NULL;
    if (params->dataset_type == DATA_MNIST)
        query_set = read_data_mnist(params->query_path);
    else if (params->dataset_type == DATA_SIFT)
        query_set = read_data_sift(params->query_path);
    else
        query_set = read_data_experiment(params->query_path);
    if (query_set)
    {
        perform_query(params, dataset, query_set, hyper_index_lookup, hyper);

        for (int i = 0; i < query_set->size; i++)
            free(query_set->data[i]);

        free(query_set->data);
        free(query_set);
    }

    hyper_destroy(hyper);

    return;
}

void run_ivfflat(SearchParams* params, Dataset* dataset)
{
    // Placeholder for IVFFlat algorithm implementation
    printf("Running IVFFlat with dataset: %s\n", params->dataset_path);

    int subsetSize = findSubsetSize(dataset->size);
    createSubset(dataset, subsetSize);
    // lloydAlgorithm();

    return;
}

void run_ivfpq(SearchParams* params, Dataset* dataset)
{
    // Placeholder for IVFPQ algorithm implementation
    printf("Running IVFPQ with dataset: %s\n", params->dataset_path);
    return;
}
