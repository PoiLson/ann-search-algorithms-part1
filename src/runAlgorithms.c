#include "../include/main.h"

void lsh_index_lookup(const void* q, const struct SearchParams* params, int* approx_neighbors, double* approx_dists, int* approx_count,
                     int** range_neighbors, int* range_count, void* index_data)            
{
    struct LSH* lsh = (struct LSH*)index_data;

    for (int tbl_idx = 0; tbl_idx < lsh->L; tbl_idx++)
    {
        int q_id;
        int bucket_idx = lsh->amplified_hash_functions[tbl_idx](q, lsh, tbl_idx, &q_id);

        Node bucket = hash_table_get_bucket(lsh->hash_tables[tbl_idx], bucket_idx);
        while (bucket)
        {
            int data_idx = *(int*)bucket->key;
            void* p = bucket->data;

            if(q_id != bucket->ID)
            {
                bucket = bucket->next;
                continue;
            }

            float dist = euclidean_distance(q, p);
            printf("knn distance: %f, hashtable: %d\n", dist, tbl_idx);

            if (*approx_count < params->N || dist < approx_dists[*approx_count - 1])
            {
                if (*approx_count < params->N)
                    (*approx_count)++;


                int i = 0;
                for (i = *approx_count - 1; i > 0 && dist < approx_dists[i - 1]; i--)
                {
                    approx_neighbors[i] = approx_neighbors[i - 1];
                    approx_dists[i] = approx_dists[i - 1];
                }
                
                approx_neighbors[i] = data_idx;
                approx_dists[i] = dist;
            }

            if (params->range_search && dist <= params->R)
            {
                *range_neighbors = (int*)realloc(*range_neighbors, (*range_count + 1) * sizeof(int));

                if (*range_neighbors)
                    (*range_neighbors)[(*range_count)++] = data_idx;
                else
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    break;
                    // TODO, might need a memory cleanup if it fails
                }
            }

            bucket = bucket->next;
        }
    }
}

void run_lsh(SearchParams* params, Dataset* dataset)
{
    struct LSH* lsh = lsh_init(params, dataset);

    Dataset* query_set = read_data("query.dat");
    if (query_set)
    {
        perform_query(params, dataset, query_set, lsh_index_lookup, lsh);

        for (int i = 0; i < query_set->size; i++)
            free(query_set->data[i]);

        free(query_set->data);
        free(query_set);
    }

    lsh_destroy(lsh);

    // Previous Implementation
    // Placeholder for LSH algorithm implementation
    // lsh_init(params, dataset);
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
