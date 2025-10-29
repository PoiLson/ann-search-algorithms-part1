#include "../include/main.h"

// Modular query function, works with any index lookup function
void perform_query(const struct SearchParams* params, const struct Dataset* dataset, const struct Dataset* query_set, index_lookup lookup_func, void* index_data)
{
    // Open output file
    FILE* output_file = fopen(params->output_path, "w");
    if (!output_file)
    {
        fprintf(stderr, "Error opening output file: %s\n", params->output_path);
        return;
    }

    // Metrics accumulators
    double total_approx_time = 0.0, total_true_time = 0.0;
    int query_count = query_set->size;
    double total_af = 0.0, total_recall = 0.0;

    // Guard against mismatched dimensions between dataset and queries
    if (dataset->dimension != query_set->dimension)
    {
        fprintf(stderr, "dataset dimension (%d) != query dimension (%d). Using min dimension for distance.\n", dataset->dimension, query_set->dimension);
        exit(EXIT_FAILURE);
    }
    //choose distance function based on data type
    float (*distance_func)(const void*, const void*, const int) = (dataset->data_type == DATA_TYPE_FLOAT) ? euclidean_distance : euclidean_distance_uint8;

    // Main query loop
    // Iterate over each query in the query set
    for (int q_idx = 0; q_idx < query_set->size; q_idx++)
    {
        void* q = query_set->data[q_idx];

        int* approx_neighbors = (int*)malloc(params->N * sizeof(int));
        double* approx_dists = (double*)malloc(params->N * sizeof(double));
        int approx_count = 0;
        int* range_neighbors = NULL;
        int range_count = 0;

        for(int i = 0; i < params->N; i++)
        {
            approx_neighbors[i] = -1;
            approx_dists[i] = INFINITY;
        }

        clock_t start_approx = clock();
        // Algorithm-specific index lookup
        lookup_func(q, params, approx_neighbors, approx_dists, &approx_count, &range_neighbors, &range_count, index_data);
        clock_t end_approx = clock();

        double approx_time = (double)(end_approx - start_approx) / CLOCKS_PER_SEC;
        total_approx_time += approx_time;

          // True kNN (brute force)
        int* true_neighbors = (int*)malloc(params->N * sizeof(int));
        double* true_dists = (double*)malloc(params->N * sizeof(double));
        int true_count = 0;

        for(int i = 0; i < params->N; i++)
        {
            true_neighbors[i] = -1;
            true_dists[i] = INFINITY;
        }

        clock_t start_true = clock();
        for (int i = 0; i < dataset->size; i++)
        {
            void* p = dataset->data[i];

            float dist = distance_func(q, p, dataset->dimension);

            if (true_count < params->N || dist < true_dists[true_count - 1])
            {
                if (true_count < params->N)
                    true_count++;

                int j = 0;
                for (j = true_count - 1; j > 0 && dist < true_dists[j - 1]; j--)
                {
                    true_neighbors[j] = true_neighbors[j - 1];
                    true_dists[j] = true_dists[j - 1];
                }

                true_neighbors[j] = i;
                true_dists[j] = dist;
            }
        }
        clock_t end_true = clock();
        
        double true_time = (double)(end_true - start_true) / CLOCKS_PER_SEC;
        total_true_time += true_time;


        /* Compute approximation factor (AF) robustly. If we don't have a true or approx
         * nearest neighbor, set AF to INFINITY to indicate no valid approximation.
         */
        double af = INFINITY;
        if (true_count > 0 && approx_count > 0 && true_dists[0] > 0.0)
        {
            af = approx_dists[0] / true_dists[0];
        }

        /* Compute Recall@N: fraction of the top-true (up to N) neighbors that appear
         * in the approximate result. Iterate over top-true neighbors and check if
         * each one appears at least once in the approx list (prevents double-counting).
         */
        int recall_count = 0;
        int denom = (true_count < params->N) ? true_count : params->N;
        for (int i = 0; i < denom; i++)
        {
            int t_idx = true_neighbors[i];
            for (int j = 0; j < approx_count; j++)
            {
                if (t_idx == approx_neighbors[j])
                {
                    recall_count++;
                    break; /* don't double-count this true neighbor */
                }
            }
        }
        double recall = (denom > 0) ? ((double)recall_count / (double)denom) : 0.0;

        total_af += af;
        total_recall += recall;

        // Output
        fprintf(output_file, "%s\n", params->algorithm == ALG_LSH ? "LSH" :
                params->algorithm == ALG_HYPERCUBE ? "Hypercube" :
                params->algorithm == ALG_IVFFLAT ? "IVFFlat" : "IVFPQ");
        fprintf(output_file, "Query: %d\n", q_idx);

        for (int i = 0; i < params->N && i < approx_count; i++)
        {
            fprintf(output_file, "Nearest neighbor-%d: %d\n", i + 1, approx_neighbors[i]);
            fprintf(output_file, "distanceApproximate: %f\n", approx_dists[i]);
            fprintf(output_file, "distanceTrue: %f\n", true_dists[i]);
        }

        if (params->range_search && range_count > 0)
        {
            fprintf(output_file, "R-near neighbors:\n");

            for (int i = 0; i < range_count; i++)
            {
                fprintf(output_file, "%d\n", range_neighbors[i]);
            }
        }

        free(approx_neighbors);
        free(approx_dists);
        free(range_neighbors);
        free(true_neighbors);
        free(true_dists);
    }

    // Final aggregated metrics over all queries
    if (query_count > 0)
    {
        double avg_af = total_af / (double)query_count;
        double avg_recall = total_recall / (double)query_count;
        double avg_t_approx = total_approx_time / (double)query_count;
        double avg_t_true = total_true_time / (double)query_count;
        double qps_overall = (total_approx_time > 0.0) ? ((double)query_count / total_approx_time) : 0.0;

        fprintf(output_file, "===== OVERALL METRICS =====\n");
        fprintf(output_file, "Average AF (mean over queries): %f\n", avg_af);
        fprintf(output_file, "Average Recall@N: %f\n", avg_recall);
        fprintf(output_file, "Average tApproximate: %f\n", avg_t_approx);
        fprintf(output_file, "Average tTrue: %f\n", avg_t_true);
        fprintf(output_file, "QPS_overall: %f\n", qps_overall);
    }

    fclose(output_file);
}