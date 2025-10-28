#include "../include/main.h"

// ------------------ Diagnostics: per-run candidate count stats ------------------
// We accumulate the number of unique candidates examined per query and
// print summary statistics at lsh_destroy if LSH_CAND_STATS=1 is set.
static int *g_lsh_cand_counts = NULL;
static int g_lsh_cand_count = 0;
static int g_lsh_cand_cap = 0;

static void lsh_diag_append_count(int val)
{
    // Only track when stats are enabled to minimize overhead
    const char *stats = getenv("LSH_CAND_STATS");
    if (!(stats && stats[0] == '1'))
        return;
    if (g_lsh_cand_count >= g_lsh_cand_cap)
    {
        int new_cap = g_lsh_cand_cap > 0 ? g_lsh_cand_cap * 2 : 128;
        int *tmp = (int *)realloc(g_lsh_cand_counts, new_cap * sizeof(int));
        if (!tmp)
            return; // best-effort; skip if OOM
        g_lsh_cand_counts = tmp;
        g_lsh_cand_cap = new_cap;
    }
    g_lsh_cand_counts[g_lsh_cand_count++] = val;
}

static int cmp_int_asc(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

static void lsh_diag_print_stats(int *arr, int n, int L)
{
    if (!arr || n <= 0)
        return;
    long long sum = 0;
    int mn = arr[0], mx = arr[0];
    for (int i = 0; i < n; ++i)
    {
        if (arr[i] < mn)
            mn = arr[i];
        if (arr[i] > mx)
            mx = arr[i];
        sum += arr[i];
    }
    double mean = (double)sum / (double)n;

    // median: sort a copy
    int *copy = (int *)malloc(n * sizeof(int));
    if (!copy)
    {
        printf("[LSH] Candidate stats: n=%d, min=%d, max=%d, mean=%.2f (L=%d)\n", n, mn, mx, mean, L);
        return;
    }
    memcpy(copy, arr, n * sizeof(int));
    qsort(copy, n, sizeof(int), cmp_int_asc);
    double median = (n % 2) ? (double)copy[n / 2] : 0.5 * ((double)copy[n / 2 - 1] + (double)copy[n / 2]);

    printf("[LSH] Candidate stats: n=%d, min=%d, max=%d, mean=%.2f, median=%.2f (L=%d)\n",
           n, mn, mx, mean, median, L);
    free(copy);
}

// Calculates ID of a vector of the dataset
// And also the hash value (g(p)) for that vector
// Core implementation of LSH hash function
// int hash_func_impl_lsh(const void* p, const LSH* lsh, int table_index, uint64_t* outID)
// {
//     // assert(lsh != nullptr);
//     // assert(lsh->num_of_buckets > 0);

//     uint64_t M = lsh->num_of_buckets;
//     uint64_t id = 0;

//     // Use the hash parameters corresponding to this table
//     const LSH_hash_function* table_hash_params = lsh->hash_params[table_index];

//     for (int i = 0; i < lsh->k; i++)
//     {
//         // Compute dot product in double precision for stability
//         double func = 0.0;
//         if (lsh->data_type == DATA_TYPE_FLOAT)
//             func = dot_product_float(table_hash_params[i].v, (const float*)p, lsh->d);
//         else
//             func = dot_product_float_int(table_hash_params[i].v, (const int*)p, lsh->d);

//         // Apply shift and bucket width
//         double val = (func + (double)table_hash_params[i].t) / (double)lsh->w;

//         // Use floor for double (handles negatives correctly)
//         int64_t h_i = (int64_t)floor(val);

//         // Linear combination with random coefficient
//         int64_t r_i = lsh->linear_combinations[table_index][i];

//         // Promote to 128-bit to avoid overflow
//         __int128 prod = (__int128)r_i * (__int128)h_i;

//         // Normalize modulo M (always non-negative)
//         uint64_t modM = (uint64_t)((prod % (int64_t)M + (int64_t)M) % (int64_t)M);

//         // Accumulate into ID
//         id = (id + modM) % M;
//     }

//     if (outID)
//         *outID = id; // bounded by M

//     // Map to actual table bucket
//     int bucket_idx = (int)(id % (uint64_t)lsh->table_size);
//     return bucket_idx;
// }

// Compute both hash value (table index) and fingerprint (ID)
int hash_func_impl_lsh(const void *p, const LSH *lsh, int table_index, uint64_t *outID)
{
    // The prime modulus for universal hashing
    const uint64_t M = lsh->num_of_buckets; // should be 2^32 - 5

    uint64_t hash_val = 0; // for h₁ → table index
    uint64_t id_val = 0;   // for h₂ → fingerprint

    const LSH_hash_function *table_hash_params = lsh->hash_params[table_index];

    for (int i = 0; i < lsh->k; i++)
    {
        // ---- Compute LSH projection: h_i(v) = floor((a·v + b)/w)
        double func = 0.0;
        if (lsh->data_type == DATA_TYPE_FLOAT)
            func = dot_product_float(table_hash_params[i].v, (const float *)p, lsh->d);
        else
            func = dot_product_float_int(table_hash_params[i].v, (const int *)p, lsh->d);

        double val = (func + (double)table_hash_params[i].t) / (double)lsh->w;
        int64_t h_i = (int64_t)floor(val); // handle negatives properly

        // ---- Universal hashing linear coefficients
        int64_t r1_i = lsh->linear_combinations_1[table_index][i]; // for hash value
        int64_t r2_i = lsh->linear_combinations_2[table_index][i]; // for ID

        // ---- Compute modular products safely in 128-bit space
        __int128 prod1 = (__int128)r1_i * (__int128)h_i;
        __int128 prod2 = (__int128)r2_i * (__int128)h_i;

        // Reduce modulo prime (always non-negative)
        uint64_t mod1 = (uint64_t)((prod1 % (int64_t)M + (int64_t)M) % (int64_t)M);
        uint64_t mod2 = (uint64_t)((prod2 % (int64_t)M + (int64_t)M) % (int64_t)M);

        // ---- Accumulate mod prime
        hash_val = (hash_val + mod1) % M;
        id_val = (id_val + mod2) % M;
    }

    // ---- Output fingerprint (h₂)
    if (outID)
        *outID = id_val; // this is the "h2" in the manual (fingerprint)

    // ---- Final hash table index (h₁)
    int bucket_idx = (int)(hash_val % (uint64_t)lsh->table_size);
    return bucket_idx;
}

// Wrapper function
int hash_function_lsh(HashTable ht, void *data, uint64_t *ID)
{
    LSH *lsh_ctx = (LSH *)hash_table_get_algorithm_context(ht);
    if (!lsh_ctx || lsh_ctx->num_of_buckets == 0)
    {
        if (ID)
            *ID = 0; // safer sentinel than -1 in uint64_t
        return 0;
    }

    int t_idx = hash_table_get_index(ht);
    return hash_func_impl_lsh(data, lsh_ctx, t_idx, ID);
}

void init_linear_combinations(LSH *lsh)
{
    lsh->linear_combinations_1 = malloc(lsh->L * sizeof(int32_t *));
    lsh->linear_combinations_2 = malloc(lsh->L * sizeof(int32_t *));
    if (!lsh->linear_combinations_1 || !lsh->linear_combinations_2)
    {
        fprintf(stderr, "Memory allocation failed for linear combinations.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < lsh->L; i++)
    {
        lsh->linear_combinations_1[i] = malloc(lsh->k * sizeof(int32_t));
        lsh->linear_combinations_2[i] = malloc(lsh->k * sizeof(int32_t));

        if (!lsh->linear_combinations_1[i] || !lsh->linear_combinations_2[i])
        {
            fprintf(stderr, "Memory allocation failed for linear combinations row.\n");
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < lsh->k; j++)
        {
            uint32_t r1 = (uint32_t)(rand() % (R_RANGE - 1)) + 1;
            uint32_t r2 = (uint32_t)(rand() % (R_RANGE - 1)) + 1;

            lsh->linear_combinations_1[i][j] = (int32_t)r1;
            lsh->linear_combinations_2[i][j] = (int32_t)r2;
        }
    }
}

LSH *lsh_init(const struct SearchParams *params, const struct Dataset *dataset)
{
    // Allocate memory for LSH structure and set parameters
    LSH *lsh = (LSH *)malloc(sizeof(LSH));
    lsh->d = dataset->dimension;
    lsh->L = params->L;
    lsh->k = params->k;
    lsh->w = params->w;
    lsh->dataset_size = dataset->size;
    // lsh->table_size = dataset->size / 4;
    // lsh->num_of_buckets = (1 << 16) - 4; // need a larger number?

    lsh->table_size = nearest_prime(dataset->size / 4);
    // Set M to the largest 32-bit prime (2^32 - 5) to avoid overflow in older impls
    lsh->num_of_buckets = 4294967291ULL;

    // Store data type and select distance function
    lsh->data_type = dataset->data_type;
    if (dataset->data_type == DATA_TYPE_FLOAT)
        lsh->distance = euclidean_distance; // float-based distance
    else
        lsh->distance = euclidean_distance_int; // int-based distance

    // Allocate memory for per-table hash parameters (L x k)
    lsh->hash_params = (LSH_hash_function **)malloc(lsh->L * sizeof(LSH_hash_function *));
    if (!lsh->hash_params)
    {
        lsh_destroy(lsh);
        exit(EXIT_FAILURE);
    }

    // For each table, generate K independent hash functions (v, t)
    for (int tbl = 0; tbl < lsh->L; tbl++)
    {
        lsh->hash_params[tbl] = (LSH_hash_function *)malloc(lsh->k * sizeof(LSH_hash_function));
        if (!lsh->hash_params[tbl])
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < lsh->k; i++)
        {
            lsh->hash_params[tbl][i].v = (float *)malloc(lsh->d * sizeof(float));
            if (!lsh->hash_params[tbl][i].v)
            {
                lsh_destroy(lsh);
                exit(EXIT_FAILURE);
            }
            generate_random_vector(lsh->hash_params[tbl][i].v, lsh->d);
            // normalize_vector(lsh->hash_params[tbl][i].v, lsh->d);
            // Normalize projection vector so that its components sum to 1 —
            // this makes the projection vector scale-invariant in a different way
            // than the usual L2 normalization. Use case: when you want the
            // projection to represent a weighted average of coordinates.
            // normalize_vector_sum(lsh->hash_params[tbl][i].v, lsh->d);

            // Calculate t uniformly in [0, w)
            float zero = 0.0f;
            float wval = lsh->w;
            lsh->hash_params[tbl][i].t = uniform_distribution(&zero, &wval);
        }
    }

    init_linear_combinations(lsh);

    // create a hash table for each hash table in LSH
    lsh->hash_tables = (HashTable *)malloc(lsh->L * sizeof(HashTable));

    for (int i = 0; i < lsh->L; i++)
    {
        lsh->hash_tables[i] = hash_table_create(lsh->table_size, sizeof(int), NULL, compare_vectors, hash_function_lsh, lsh, i, &(dataset->dimension));

        if (!lsh->hash_tables[i])
        {
            lsh_destroy(lsh);
            exit(EXIT_FAILURE);
        }
    }

    // insert all points in all hash tables
    for (int i = 0; i < dataset->size; i++)
    {
        // Verify point is valid before insertion
        if (!dataset->data || !dataset->data[i])
        {
            printf("Warning: dataset[%d] is NULL\n", i);
            continue;
        }
        for (int j = 0; j < lsh->L; j++)
        {
            // printf("INSERTING INSIDE THE HASHTABLE LSH, i = %d\n", i);
            hash_table_insert(lsh->hash_tables[j], &i, dataset->data[i]);
        }
    }

    // Optional: dump per-point bucket assignments for each table to a file
    // Set env var LSH_DUMP_BUCKETS to a filepath to enable
    const char *dump_path = getenv("LSH_DUMP_BUCKETS");
    if (dump_path && dump_path[0] != '\0')
    {
        FILE *df = fopen(dump_path, "w");
        if (df)
        {
            fprintf(df, "LSH_DUMP\n");
            fprintf(df, "L %d\nK %d\nW %.6f\nTABLE_SIZE %d\n", lsh->L, lsh->k, (double)lsh->w, lsh->table_size);
            fprintf(df, "DATASET_SIZE %d\nDIM %d\nDATA_TYPE %s\n",
                    lsh->dataset_size,
                    lsh->d,
                    (lsh->data_type == DATA_TYPE_FLOAT) ? "FLOAT" : "INT");

            for (int tbl = 0; tbl < lsh->L; ++tbl)
            {
                fprintf(df, "TABLE %d\n", tbl);
                for (int i = 0; i < lsh->dataset_size; ++i)
                {
                    uint64_t id = 0ULL;
                    int bucket_idx = hash_func_impl_lsh((const void *)dataset->data[i], lsh, tbl, &id);
                    fprintf(df, "IDX %d BUCKET %d ID %llu", i, bucket_idx, (unsigned long long)id);
                    // Dump ALL coordinates for this vector (full dimensionality)
                    fprintf(df, " COORD");
                    if (lsh->data_type == DATA_TYPE_FLOAT)
                    {
                        float *v = (float *)dataset->data[i];
                        for (int c = 0; c < lsh->d; ++c)
                            fprintf(df, " %g", (double)v[c]);
                    }
                    else
                    {
                        int *v = (int *)dataset->data[i];
                        for (int c = 0; c < lsh->d; ++c)
                            fprintf(df, " %d", v[c]);
                    }
                    fprintf(df, "\n");
                }
            }
            fclose(df);
        }
        else
        {
            fprintf(stderr, "Warning: failed to open dump path '%s' for writing.\n", dump_path);
        }
    }
    return lsh;
}

void lsh_index_lookup(const void *q, const struct SearchParams *params, int *approx_neighbors, double *approx_dists, int *approx_count,
                      int **range_neighbors, int *range_count, void *index_data)
{
    // Cast index_data to LSH structure
    struct LSH *lsh = (struct LSH *)index_data;

    // Use a visited array for O(1) duplicate detection across all tables
    bool *visited = (bool *)calloc(lsh->dataset_size, sizeof(bool));
    if (!visited)
    {
        fprintf(stderr, "Failed to allocate visited array.\n");
        return;
    }

    // Diagnostic: count how many UNIQUE candidates we actually evaluated
    int unique_candidates = 0;

    // Range neighbors dynamic capacity (grow in chunks instead of per-item realloc)
    const int RANGE_ALLOC_CHUNK = 128;
    int range_capacity = (*range_neighbors && *range_count > 0) ? *range_count : 0;

    int flagged = 0;
    // For each hash table, compute the bucket index for the query point
    for (int tbl_idx = 0; tbl_idx < lsh->L; tbl_idx++)
    {
        uint64_t q_id = 0ULL;
        // printf("INSIDE OF LSH INDEX LOOKUP\n");
        int bucket_idx = hash_func_impl_lsh(q, lsh, tbl_idx, &q_id);

        int bucket_count = 0;
        const HTEntry *bucket = hash_table_get_bucket_entries(lsh->hash_tables[tbl_idx], bucket_idx, &bucket_count);
        for (int bi = 0; bi < bucket_count; ++bi)
        {
            int data_idx = *(int *)bucket[bi].key;
            void *p = bucket[bi].data;

            if (bucket[bi].ID != q_id)
                continue;

            // Skip duplicate candidates across tables using visited array
            if (data_idx >= 0 && data_idx < lsh->dataset_size && visited[data_idx])
                continue;

            // Mark as visited now to avoid reprocessing and count as unique candidate
            if (data_idx >= 0 && data_idx < lsh->dataset_size)
            {
                visited[data_idx] = true;
                unique_candidates++;
            }

            // Compute distance for this candidate using type-aware function
            float dist = lsh->distance(q, p, lsh->d);

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
                // Ensure capacity and append without per-item realloc
                if (*range_count >= range_capacity)
                {
                    int new_capacity = range_capacity + RANGE_ALLOC_CHUNK;
                    int *new_buf = (int *)realloc(*range_neighbors, new_capacity * sizeof(int));
                    if (!new_buf)
                    {
                        fprintf(stderr, "Memory reallocation failed for range neighbors.\n");
                        free(*range_neighbors);
                        *range_neighbors = NULL;
                        *range_count = 0;
                        break;
                    }
                    *range_neighbors = new_buf;
                    range_capacity = new_capacity;
                }

                // Append this neighbor
                (*range_neighbors)[(*range_count)++] = data_idx;
            }
        }
    }

    // Diagnostic print (opt-in): total unique candidates examined for this query
    // Enable by setting environment variable LSH_PRINT_CANDIDATES=1
    const char *lsh_diag = getenv("LSH_PRINT_CANDIDATES");
    if (lsh_diag && lsh_diag[0] == '1')
    {
        printf("[LSH] Unique candidates examined: %d (L=%d)\n", unique_candidates, lsh->L);
    }

    // Append to per-run stats (if enabled)
    lsh_diag_append_count(unique_candidates);

    // cleanup if no range neighbors found
    if (*range_count == 0)
    {
        free(*range_neighbors);
        *range_neighbors = NULL;
    }

    free(visited);
}

void lsh_destroy(struct LSH *lsh)
{
    if (!lsh)
        return;

    // Print aggregated candidate stats before freeing context
    if (g_lsh_cand_count > 0)
    {
        const char *stats = getenv("LSH_CAND_STATS");
        if (stats && stats[0] == '1')
        {
            lsh_diag_print_stats(g_lsh_cand_counts, g_lsh_cand_count, lsh->L);
        }
        free(g_lsh_cand_counts);
        g_lsh_cand_counts = NULL;
        g_lsh_cand_count = 0;
        g_lsh_cand_cap = 0;
    }

    // Free hash params and their vectors
    if (lsh->hash_params)
    {
        for (int tbl = 0; tbl < lsh->L; tbl++)
        {
            if (lsh->hash_params[tbl])
            {
                for (int i = 0; i < lsh->k; i++)
                {
                    if (lsh->hash_params[tbl][i].v)
                        free(lsh->hash_params[tbl][i].v);
                }
                free(lsh->hash_params[tbl]);
            }
        }
        free(lsh->hash_params);
    }

    // Free linear combinations
    if (lsh->linear_combinations_1)
    {
        for (int i = 0; i < lsh->L; i++)
        {
            if (lsh->linear_combinations_1[i])
                free(lsh->linear_combinations_1[i]);
        }
        free(lsh->linear_combinations_1);
    }

    if (lsh->linear_combinations_2)
    {
        for (int i = 0; i < lsh->L; i++)
        {
            if (lsh->linear_combinations_2[i])
                free(lsh->linear_combinations_2[i]);
        }
        free(lsh->linear_combinations_2);
    }

    // Destroy hash tables (guard duplicates) and free array
    if (lsh->hash_tables)
    {
        for (int i = 0; i < lsh->L; i++)
        {
            if (lsh->hash_tables[i])
            {
                int duplicate = 0;
                for (int j = 0; j < i; j++)
                {
                    if (lsh->hash_tables[j] == lsh->hash_tables[i])
                    {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate)
                {
                    hash_table_destroy(lsh->hash_tables[i]);
                }
                lsh->hash_tables[i] = NULL;
            }
        }
        free(lsh->hash_tables);
    }

    free(lsh);
}