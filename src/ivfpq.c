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
    
    // Initialize centroids using KMeans++
    int *assignments = (int *)malloc(n_points * sizeof(int));
    bool *is_centroid = (bool *)calloc(n_points, sizeof(bool));
    double *min_distances = (double *)malloc(n_points * sizeof(double));
    
    // Step 1: Choose first centroid randomly
    int first_idx = rand() % n_points;
    memcpy(centroids[0], subspace_data[first_idx], d_sub * sizeof(float));
    is_centroid[first_idx] = true;
    
    // Step 2: Choose remaining centroids using KMeans++ (weighted by distance^2)
    for (int c = 1; c < s && c < n_points; c++) {
        // Compute D(x)^2 for each point (distance to nearest existing centroid)
        double total_distance = 0.0;
        for (int i = 0; i < n_points; i++) {
            if (is_centroid[i]) {
                min_distances[i] = 0.0;
                continue;
            }
            
            double min_dist = DBL_MAX;
            for (int cc = 0; cc < c; cc++) {
                double dist = 0.0;
                for (int j = 0; j < d_sub; j++) {
                    double diff = subspace_data[i][j] - centroids[cc][j];
                    dist += diff * diff;
                }
                if (dist < min_dist) {
                    min_dist = dist;
                }
            }
            min_distances[i] = min_dist;
            total_distance += min_dist;
        }
        
        // Choose next centroid with probability proportional to D(x)^2
        double random_val = ((double)rand() / RAND_MAX) * total_distance;
        double cumulative = 0.0;
        int chosen_idx = 0;
        
        for (int i = 0; i < n_points; i++) {
            if (is_centroid[i]) continue;
            cumulative += min_distances[i];
            if (cumulative >= random_val) {
                chosen_idx = i;
                break;
            }
        }
        
        memcpy(centroids[c], subspace_data[chosen_idx], d_sub * sizeof(float));
        is_centroid[chosen_idx] = true;
    }
    
    free(is_centroid);
    free(min_distances);
    
    // Lloyd's iterations
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
        
        // Recompute centroids
        int *counts = (int *)calloc(s, sizeof(int));
        for (int c = 0; c < s; c++) {
            memset(centroids[c], 0, d_sub * sizeof(float));
        }
        
        for (int i = 0; i < n_points; i++) {
            int c = assignments[i];
            counts[c]++;
            for (int j = 0; j < d_sub; j++) {
                centroids[c][j] += subspace_data[i][j];
            }
        }
        
        for (int c = 0; c < s; c++) {
            if (counts[c] > 0) {
                for (int j = 0; j < d_sub; j++) {
                    centroids[c][j] /= counts[c];
                }
            }
        }
        free(counts);
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
    
    // Step 3-7: For each cluster, compute residuals and train PQ
    printf("Step 3-7: Computing residuals and training product quantizers...\n");
    
    for (int c = 0; c < k_clusters; c++) {
        InvertedList *ivf_list = &ivf_temp->lists[c];
        int n_points = ivf_list->count;
        
        if (n_points < 10) continue;  // Skip small clusters
        
        // Allocate residuals for this cluster
        float **residuals = (float **)malloc(n_points * sizeof(float *));
        for (int i = 0; i < n_points; i++) {
            residuals[i] = (float *)malloc(dataset->dimension * sizeof(float));
            compute_residual(ivf_list->points[i], index->centroids[c], 
                           dataset->data_type, dataset->dimension, residuals[i]);
        }
        
        // Step 4-5: Split residuals into M parts and train subspace quantizers
        for (int m = 0; m < M; m++) {
            // Extract subspace m from all residuals
            float **subspace_data = (float **)malloc(n_points * sizeof(float *));
            for (int i = 0; i < n_points; i++) {
                subspace_data[i] = (float *)malloc(index->pq.d_sub * sizeof(float));
                for (int j = 0; j < index->pq.d_sub; j++) {
                    subspace_data[i][j] = residuals[i][m * index->pq.d_sub + j];
                }
            }
            
            // Train Lloyd's on this subspace (only for first cluster, reuse for others)
            if (c == 0 || index->pq.subspace_centroids[m] == NULL) {
                if (index->pq.subspace_centroids[m] == NULL) {
                    index->pq.subspace_centroids[m] = run_lloyd_on_subspace(
                        subspace_data, n_points, index->pq.d_sub, index->pq.s, 20);
                }
            }
            
            // Free subspace data
            for (int i = 0; i < n_points; i++) {
                free(subspace_data[i]);
            }
            free(subspace_data);
        }
        
        // Free residuals
        for (int i = 0; i < n_points; i++) {
            free(residuals[i]);
        }
        free(residuals);
    }
    
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
    double *centroid_dists = (double *)malloc(nprobe * sizeof(double));
    int *centroid_ids = (int *)malloc(nprobe * sizeof(int));
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
            float approx_dist = 0.0f;
            uint8_t *pq_code = list->entries[i].pq_code;
            
            for (int m = 0; m < index->pq.M; m++) {
                approx_dist += lookup_table[m][pq_code[m]];
            }
            
            approx_dist = sqrtf(approx_dist);  // Take square root for Euclidean distance
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
    heap_extract_sorted(topN, approx_neighbors, approx_dists);
    heap_destroy(topN);
    
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