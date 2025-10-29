#include "../include/main.h"

void run_lsh(SearchParams* params, Dataset* dataset)
{
    struct LSH* lsh = lsh_init(params, dataset);

    if (!lsh)
    {
        fprintf(stderr, "Failed to build LSH index.\n");
        return;
    }
    
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
    if (!hyper)
    {
        fprintf(stderr, "Failed to build Hypercube index.\n");
        return;
    }

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
    // Build the index
    IVFFlatIndex* ivf_index = ivfflat_init(dataset, params->kclusters);
    if (!ivf_index)
    {
        fprintf(stderr, "Failed to build IVFFlat index.\n");
        return;
    }

    // Compute silhouette (prints results internally)
    // computeSilhouette(ivf_index, dataset);

    Dataset* query_set = NULL;
    if (params->dataset_type == DATA_MNIST)
        query_set = read_data_mnist(params->query_path);
    else if (params->dataset_type == DATA_SIFT)
        query_set = read_data_sift(params->query_path);
    else
        query_set = read_data_experiment(params->query_path);

    if (query_set)
    {
        perform_query(params, dataset, query_set, ivfflat_index_lookup, ivf_index);

        for (int i = 0; i < query_set->size; i++)
            free(query_set->data[i]);

        free(query_set->data);
        free(query_set);
    }

    ivfflat_destroy(ivf_index);

    return;
}

void run_ivfpq(SearchParams* params, Dataset* dataset)
{
    // Placeholder for IVFPQ algorithm implementation
    printf("Running IVFPQ with dataset: %s\n", params->dataset_path);

    // Build the index
    ivfpq_init();
    // if (!ivf_index)
    // {
    //     fprintf(stderr, "Failed to build IVFFlat index.\n");
    //     return;
    // }

    // Dataset* query_set = NULL;
    // if (params->dataset_type == DATA_MNIST)
    //     query_set = read_data_mnist(params->query_path);
    // else if (params->dataset_type == DATA_SIFT)
    //     query_set = read_data_sift(params->query_path);
    // else
    //     query_set = read_data_experiment(params->query_path);

    // if (query_set)
    // {
    //     perform_query(params, dataset, query_set, ivfpq_index_lookup, ivf_index);

    //     for (int i = 0; i < query_set->size; i++)
    //         free(query_set->data[i]);

    //     free(query_set->data);
    //     free(query_set);
    // }

    // ivfpq_destroy(ivf_index);

    return;
}
