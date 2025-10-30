#include "../include/main.h"
#include "../include/minheap.h"
#include <math.h>
#include <float.h>
#include <string.h>

// Helper: Split vector into M parts
static inline void split_vector_into_parts(const void *vec, DataType data_type, int d, int M, float **parts) {
    int d_sub = d / M;
    
    if (data_type == DATA_TYPE_FLOAT) {
        const float *fvec = (const float *)vec;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < d_sub; j++) {
                parts[i][j] = fvec[i * d_sub + j];
            }
        }
    } else {  // DATA_TYPE_UINT8
        const uint8_t *uvec = (const uint8_t *)vec;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < d_sub; j++) {
                parts[i][j] = (float)uvec[i * d_sub + j];
            }
        }
    }
}

// Helper: Compute residual vector r(x) = x - c(x)
static inline void compute_residual(const void *vec, const float *centroid, DataType data_type, int d, float *residual) {
    if (data_type == DATA_TYPE_FLOAT) {
        const float *fvec = (const float *)vec;
        for (int j = 0; j < d; j++) {
            residual[j] = fvec[j] - centroid[j];
        }
    } else {  // DATA_TYPE_UINT8
        const uint8_t *uvec = (const uint8_t *)vec;
        for (int j = 0; j < d; j++) {
            residual[j] = (float)uvec[j] - centroid[j];
        }
    }
}

// Helper: Run Lloyd's on a subspace to get s centroids
static float** run_lloyd_on_subspace(float **subspace_data, int n_points, int d_sub, int s, int max_iters) {
    if (n_points == 0 || d_sub == 0) return NULL;
    
    // Allocate centroids
    float **centroids = (float **)malloc(s * sizeof(float *));
    for (int i = 0; i < s; i++) {
        centroids[i] = (float *)calloc(d_sub, sizeof(float));
    }
    
    // Initialize centroids with simple random sampling (much faster than KMeans++)
    // For PQ training, random init works well enough with multiple Lloyd iterations
    int *assignments = (int *)malloc(n_points * sizeof(int));
    
    // Choose s random unique points as initial centroids
    bool *selected = (bool *)calloc(n_points, sizeof(bool));
    int num_selected = 0;
    
    while (num_selected < s && num_selected < n_points) {
        int idx = rand() % n_points;
        if (!selected[idx]) {
            memcpy(centroids[num_selected], subspace_data[idx], d_sub * sizeof(float));
            selected[idx] = true;
            num_selected++;
        }
    }
    
    free(selected);
    
    // Lloyd's iterations with convergence checking (like IVFFlat)
    double epsilon = 1e-4;
    bool changed_any = false;
    for (int iter = 0; iter < max_iters; iter++) {
        // Assign points to nearest centroid
        for (int i = 0; i < n_points; i++) {
            float best_dist = FLT_MAX;
            int best_c = 0;
            for (int c = 0; c < s; c++) {
                float dist = 0.0f;
                for (int j = 0; j < d_sub; j++) {
                    float diff = subspace_data[i][j] - centroids[c][j];
                    dist += diff * diff;
                }
                if (dist < best_dist) {
                    best_dist = dist;
                    best_c = c;
                }
            }
            assignments[i] = best_c;
        }
        
        // Recompute centroids and check for convergence
        int *counts = (int *)calloc(s, sizeof(int));
        float **new_centroids = (float **)malloc(s * sizeof(float *));
        for (int c = 0; c < s; c++) {
            new_centroids[c] = (float *)calloc(d_sub, sizeof(float));
        }
        
        // Sum points assigned to each centroid
        for (int i = 0; i < n_points; i++) {
            int c = assignments[i];
            counts[c]++;
            for (int j = 0; j < d_sub; j++) {
                new_centroids[c][j] += subspace_data[i][j];
            }
        }
        
        // Average and check for convergence (like IVFFlat recompute_centroids)
        changed_any = false;
        for (int c = 0; c < s; c++) {
            if (counts[c] > 0) {
                for (int j = 0; j < d_sub; j++) {
                    new_centroids[c][j] /= counts[c];
                }
            }
            
            // Check centroid shift (Euclidean distance)
            double shift = 0.0;
            for (int j = 0; j < d_sub; j++) {
                double diff = new_centroids[c][j] - centroids[c][j];
                shift += diff * diff;
            }
            shift = sqrt(shift);
            
            if (shift > epsilon) {
                changed_any = true;
            }
            
            // Replace old centroid with new
            free(centroids[c]);
            centroids[c] = new_centroids[c];
        }
        
        free(new_centroids);
        free(counts);
        
        // Early stopping if converged (like IVFFlat)
        if (!changed_any) {
            printf("Lloyd's converged in %d iterations\n", iter + 1);
            break;
        }
    }
    if (changed_any) {
        printf("----Lloyd's converged in %d iterations\n", max_iters);
    }

    free(assignments);
    return centroids;
}

// Helper: Find nearest centroid in a subspace
static inline int find_nearest_subspace_centroid(const float *subvec, float **centroids, int s, int d_sub) {
    float best_dist = FLT_MAX;
    int best_idx = 0;
    
    for (int i = 0; i < s; i++) {
        float dist = 0.0f;
        for (int j = 0; j < d_sub; j++) {
            float diff = subvec[j] - centroids[i][j];
            dist += diff * diff;
        }
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

// IVFPQ Initialization following the algorithm from the image
IVFPQIndex* ivfpq_init(Dataset *dataset, int k_clusters, int M, int nbits) {
    if (!dataset || k_clusters <= 0 || M <= 0 || nbits <= 0) {
        fprintf(stderr, "Invalid IVFPQ parameters\n");
        return NULL;
    }
    
    if (dataset->dimension % M != 0) {
        fprintf(stderr, "Dimension %d must be divisible by M=%d\n", dataset->dimension, M);
        return NULL;
    }
    
    printf("Building IVFPQ index: k=%d, M=%d, nbits=%d\n", k_clusters, M, nbits);
    
    IVFPQIndex *index = (IVFPQIndex *)malloc(sizeof(IVFPQIndex));
    if (!index) return NULL;
    
    index->k = k_clusters;
    index->d = dataset->dimension;
    index->data_type = dataset->data_type;
    index->dataset = dataset;
    
    // Step 1 & 2: Run Lloyd's to get coarse centroids (reuse IVFFlat logic)
    printf("Step 1-2: Computing coarse centroids with Lloyd's algorithm...\n");
    IVFFlatIndex *ivf_temp = ivfflat_init(dataset, k_clusters);
    if (!ivf_temp) {
        free(index);
        return NULL;
    }
    
    // Copy centroids from IVFFlat
    index->centroids = ivf_temp->centroids;
    ivf_temp->centroids = NULL;  // Prevent double-free
    
    // Initialize PQ configuration
    index->pq.M = M;
    index->pq.nbits = nbits;
    index->pq.s = 1 << nbits;  // s = 2^nbits
    index->pq.d_sub = dataset->dimension / M;
    
    printf("PQ config: M=%d subspaces, s=%d centroids per subspace, d_sub=%d\n", 
           index->pq.M, index->pq.s, index->pq.d_sub);
    
    // Allocate subspace centroids [M][s][d_sub]
    index->pq.subspace_centroids = (float ***)malloc(M * sizeof(float **));
    for (int i = 0; i < M; i++) {
        index->pq.subspace_centroids[i] = NULL;
    }
    
    // Allocate inverted lists
    index->lists = (IVFPQList *)calloc(k_clusters, sizeof(IVFPQList));
    for (int i = 0; i < k_clusters; i++) {
        index->lists[i].cluster_id = i;
        index->lists[i].count = 0;
        index->lists[i].capacity = 0;
        index->lists[i].entries = NULL;
    }
    
    // Step 3-5: Collect residuals from ALL clusters and train global PQ codebooks
    printf("Step 3-5: Computing residuals from all clusters and training PQ codebooks...\n");
    
    // First, count total residuals and decide on sampling
    int total_available = dataset->size;
    
    // Sample at most 10000 residuals for faster training
    int max_train_samples = (total_available < 10000) ? total_available : 10000;
    int sample_rate = (total_available > max_train_samples) ? (total_available / max_train_samples) : 1;
    
    printf("Collecting %d/%d residual vectors for PQ training (sample rate: 1/%d)...\n", 
           max_train_samples, total_available, sample_rate);
    
    float **all_residuals = (float **)malloc(max_train_samples * sizeof(float *));
    int residual_idx = 0;
    
    for (int c = 0; c < k_clusters && residual_idx < max_train_samples; c++) {
        InvertedList *ivf_list = &ivf_temp->lists[c];
        for (int i = 0; i < ivf_list->count && residual_idx < max_train_samples; i++) {
            // Sample points based on sample_rate
            if (i % sample_rate != 0) continue;
            
            all_residuals[residual_idx] = (float *)malloc(dataset->dimension * sizeof(float));
            compute_residual(ivf_list->points[i], index->centroids[c],
                           dataset->data_type, dataset->dimension, all_residuals[residual_idx]);
            residual_idx++;
        }
    }
    
    int actual_samples = residual_idx;
    printf("Collected %d residual samples.\n", actual_samples);
    
    // Train each subspace quantizer on the sampled residuals
    printf("Training %d subspace quantizers...\n", M);
    for (int m = 0; m < M; m++) {
        printf("  Training subspace %d/%d...\n", m+1, M);
        fflush(stdout);
        
        // Extract subspace m from all residuals
        float **subspace_data = (float **)malloc(actual_samples * sizeof(float *));
        for (int i = 0; i < actual_samples; i++) {
            subspace_data[i] = (float *)malloc(index->pq.d_sub * sizeof(float));
            for (int j = 0; j < index->pq.d_sub; j++) {
                subspace_data[i][j] = all_residuals[i][m * index->pq.d_sub + j];
            }
        }
        
        // Train Lloyd's on this subspace (use fewer iterations for speed)
        index->pq.subspace_centroids[m] = run_lloyd_on_subspace(
            subspace_data, actual_samples, index->pq.d_sub, index->pq.s, 100);
        
        // Free subspace data
        for (int i = 0; i < actual_samples; i++) {
            free(subspace_data[i]);
        }
        free(subspace_data);
    }
    
    // Free all residuals
    for (int i = 0; i < actual_samples; i++) {
        free(all_residuals[i]);
    }
    free(all_residuals);
    printf("PQ codebooks trained successfully!\n");
    
    // Step 6-8: Encode all points and build inverted lists
    printf("Step 6-8: Encoding points with PQ and building inverted lists...\n");
    
    for (int c = 0; c < k_clusters; c++) {
        InvertedList *ivf_list = &ivf_temp->lists[c];
        IVFPQList *pq_list = &index->lists[c];
        
        pq_list->capacity = ivf_list->count;
        pq_list->entries = (IVFPQEntry *)malloc(ivf_list->count * sizeof(IVFPQEntry));
        
        for (int i = 0; i < ivf_list->count; i++) {
            // Compute residual
            float *residual = (float *)malloc(dataset->dimension * sizeof(float));
            compute_residual(ivf_list->points[i], index->centroids[c],
                           dataset->data_type, dataset->dimension, residual);
            
            // Split into M parts and encode
            uint8_t *pq_code = (uint8_t *)malloc(M * sizeof(uint8_t));
            for (int m = 0; m < M; m++) {
                float *subvec = &residual[m * index->pq.d_sub];
                pq_code[m] = (uint8_t)find_nearest_subspace_centroid(
                    subvec, index->pq.subspace_centroids[m], index->pq.s, index->pq.d_sub);
            }
            
            pq_list->entries[i].point_id = ivf_list->point_ids[i];
            pq_list->entries[i].pq_code = pq_code;
            pq_list->count++;
            
            free(residual);
        }
    }
    
    // Clean up temporary IVFFlat index
    ivfflat_destroy(ivf_temp);
    
    printf("IVFPQ index built successfully\n");
    return index;
}

// Asymmetric distance computation for IVFPQ lookup
void ivfpq_index_lookup(const void *q_void, const struct SearchParams *params,
                        int *approx_neighbors, double *approx_dists, int *approx_count,
                        int **range_neighbors, int *range_count, void *index_data) {
    IVFPQIndex *index = (IVFPQIndex *)index_data;
    int d = index->d;
    int k = index->k;
    int nprobe = params->nprobe;
    int N = params->N;
    
    if (nprobe > k) nprobe = k;
    
    // Step 1: Find nearest coarse centroids
    double *centroid_dists = (double *)malloc(k * sizeof(double));
    int *centroid_ids = (int *)malloc(k * sizeof(int));
    int selected = 0;
    
    // Convert query to float
    float *q_float = (float *)malloc(d * sizeof(float));
    if (index->data_type == DATA_TYPE_FLOAT) {
        memcpy(q_float, q_void, d * sizeof(float));
    } else {
        const uint8_t *q_u8 = (const uint8_t *)q_void;
        for (int i = 0; i < d; i++) {
            q_float[i] = (float)q_u8[i];
        }
    }
    
    // Find nprobe nearest centroids
    for (int i = 0; i < k; i++) {
        double dist = euclidean_distance(q_void, index->centroids[i], d, 
                                        index->data_type, DATA_TYPE_FLOAT);
        
        if (selected < nprobe) {
            int j = selected;
            for (; j > 0 && dist < centroid_dists[j - 1]; j--) {
                centroid_dists[j] = centroid_dists[j - 1];
                centroid_ids[j] = centroid_ids[j - 1];
            }
            centroid_dists[j] = dist;
            centroid_ids[j] = i;
            selected++;
        } else if (dist < centroid_dists[nprobe - 1]) {
            int j = nprobe - 1;
            for (; j > 0 && dist < centroid_dists[j - 1]; j--) {
                centroid_dists[j] = centroid_dists[j - 1];
                centroid_ids[j] = centroid_ids[j - 1];
            }
            centroid_dists[j] = dist;
            centroid_ids[j] = i;
        }
    }
    
    // Step 2: Create min-heap for top-N
    MinHeap *topN = heap_create(N);
    
    // Step 3: For each selected cluster, compute asymmetric distances
    for (int p = 0; p < nprobe; p++) {
        int cid = centroid_ids[p];
        IVFPQList *list = &index->lists[cid];
        
        // Compute query residual: q - c(cid)
        float *q_residual = (float *)malloc(d * sizeof(float));
        for (int i = 0; i < d; i++) {
            q_residual[i] = q_float[i] - index->centroids[cid][i];
        }
        
        // Precompute distances from query subvectors to all subspace centroids
        float **lookup_table = (float **)malloc(index->pq.M * sizeof(float *));
        for (int m = 0; m < index->pq.M; m++) {
            lookup_table[m] = (float *)malloc(index->pq.s * sizeof(float));
            float *q_sub = &q_residual[m * index->pq.d_sub];
            
            for (int c = 0; c < index->pq.s; c++) {
                float dist = 0.0f;
                for (int j = 0; j < index->pq.d_sub; j++) {
                    float diff = q_sub[j] - index->pq.subspace_centroids[m][c][j];
                    dist += diff * diff;
                }
                lookup_table[m][c] = dist;
            }
        }
        
        // Compute approximate distance for each point using PQ codes
        for (int i = 0; i < list->count; i++) {
            double approx_dist = 0.0;
            uint8_t *pq_code = list->entries[i].pq_code;
            
            for (int m = 0; m < index->pq.M; m++) {
                approx_dist += lookup_table[m][pq_code[m]];
            }
            
            // approx_dist = sqrtf(approx_dist);  // Take square root for Euclidean distance
            heap_insert(topN, list->entries[i].point_id, (double)approx_dist);
        }
        
        // Free lookup table
        for (int m = 0; m < index->pq.M; m++) {
            free(lookup_table[m]);
        }
        free(lookup_table);
        free(q_residual);
    }
    
    // Step 4: Extract results
    *approx_count = topN->size;
    int *approx_neighbors_adc = (int *)malloc((*approx_count) * sizeof(int));
    double *approx_dists_adc = (double *)malloc((*approx_count) * sizeof(double));
    heap_extract_sorted(topN, approx_neighbors_adc, approx_dists_adc);
    heap_destroy(topN);

    for (int i = 0; i < *approx_count; i++) {
        approx_neighbors[i] = approx_neighbors_adc[i];
        // Compute exact Euclidean distance to the selected neighbor
        void *point = index->dataset->data[approx_neighbors_adc[i]];
        approx_dists[i] = euclidean_distance(q_void, point, d, 
                                            index->data_type, index->data_type);
    }

    
    //sort aprox_dists and approx_neighbors based on approx_dists
    for (int i = 0; i < *approx_count - 1; i++)
    {
        for (int j = 0; j < *approx_count - i - 1; j++)
        {
            if (approx_dists[j] > approx_dists[j + 1])
            {
                // Swap distances
                double temp_dist = approx_dists[j];
                approx_dists[j] = approx_dists[j + 1];
                approx_dists[j + 1] = temp_dist;

                // Swap corresponding neighbors
                int temp_neighbor = approx_neighbors[j];
                approx_neighbors[j] = approx_neighbors[j + 1];
                approx_neighbors[j + 1] = temp_neighbor;
            }
        }
    }
    
    free(approx_neighbors_adc);
    free(approx_dists_adc);
    free(centroid_dists);
    free(centroid_ids);
    free(q_float);
}

void ivfpq_destroy(IVFPQIndex *index) {
    if (!index) return;
    
    // Free subspace centroids
    if (index->pq.subspace_centroids) {
        for (int m = 0; m < index->pq.M; m++) {
            if (index->pq.subspace_centroids[m]) {
                for (int s = 0; s < index->pq.s; s++) {
                    free(index->pq.subspace_centroids[m][s]);
                }
                free(index->pq.subspace_centroids[m]);
            }
        }
        free(index->pq.subspace_centroids);
    }
    
    // Free inverted lists
    if (index->lists) {
        for (int i = 0; i < index->k; i++) {
            if (index->lists[i].entries) {
                for (int j = 0; j < index->lists[i].count; j++) {
                    free(index->lists[i].entries[j].pq_code);
                }
                free(index->lists[i].entries);
            }
        }
        free(index->lists);
    }
    
    // Free centroids
    if (index->centroids) {
        for (int i = 0; i < index->k; i++) {
            free(index->centroids[i]);
        }
        free(index->centroids);
    }
    
    free(index);
}