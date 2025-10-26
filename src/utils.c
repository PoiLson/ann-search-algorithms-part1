#include "../include/main.h"

// Generates a random number uniformly distributed between a and b 
float uniform_distribution(void* a, void* b)
{
    float a_p = *(float*) a;
    float b_p = *(float*) b;

    return a_p + (b_p - a_p) * ((float)rand() / RAND_MAX);
}

float gaussian_distribution(void)
{
    static int haveSpare = 0;
    static float spare;
    if (haveSpare)
    {
        haveSpare = 0;
        return spare;
    }

    haveSpare = 1;
    float u, v, s;

    do {
        u = 2.0 * ((float)rand() / RAND_MAX) - 1.0;
        v = 2.0 * ((float)rand() / RAND_MAX) - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);

    s = sqrtf(-2.0 * logf(s) / s);
    spare = v * s;

    return u * s;
}

void generate_random_vector(float* v, int d)
{
    // Optimized bulk generation using Box-Muller (generates pairs)
    // This reduces the number of rand() calls and sqrt/log computations by half
    int i = 0;
    
    // Generate pairs of Gaussian random numbers using Box-Muller
    while (i < d - 1)
    {
        float u, v_temp, s;
        do {
            u = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
            v_temp = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
            s = u * u + v_temp * v_temp;
        } while (s >= 1.0f || s == 0.0f);
        
        float multiplier = sqrtf(-2.0f * logf(s) / s);
        v[i++] = u * multiplier;
        v[i++] = v_temp * multiplier;
    }
    
    // Handle odd dimension: generate one more if needed
    if (i < d)
    {
        v[i] = gaussian_distribution();
    }
}

void normalize_vector(float* v, int d)
{
    if (!v || d <= 0) return;
    float sumsq = 0.0f;
    for (int i = 0; i < d; i++)
        sumsq += v[i] * v[i];
    if (sumsq <= 0.0f)
    {
        // fallback to unit basis vector if random vector degenerates (extremely unlikely)
        for (int i = 0; i < d; i++) v[i] = 0.0f;
        v[0] = 1.0f;
        return;
    }
    float inv = 1.0f / sqrtf(sumsq);
    for (int i = 0; i < d; i++)
        v[i] *= inv;
}

int dot_product_int(const int* a, const int* b, int d)
{
    int sum = 0;
    for(int i = 0; i < d; i++)
        sum += a[i] * b[i];

    return sum;
}

float dot_product_float(const float* a, const float* b, int d)
{
    float sum = 0.0;
    for(int i = 0; i < d; i++)
    {
        sum += a[i] * b[i];
    }

    return sum;
}

// dot_product_float_int moved to utils.h as static inline for performance

float euclidean_distance(const void* a, const void* b, const int dimension)
{
    float sum = 0.0;
    float* a_p = (float*)a;
    float* b_p = (float*)b;

    for(int i = 0; i < dimension; i++)
    {
        float diff = a_p[i] - b_p[i];
        sum += diff * diff;
    }

    return sqrt(sum);
}

// euclidean_distance_int moved to utils.h as static inline for performance

int hamming_distance(const int* a, const int* b, int d)
{
    int count = 0;

    for(int i = 0; i < d; i++)
    {
        if(a[i] != b[i])
            count++;
    }
    return count;
}


// returns the first 'probes' neighbors from point 'a' sorted by hamming distance
void get_hamming_neighbors(uint64_t bucket, int probes, int kproj, uint64_t* neighbors)
{
    uint64_t mask = 0;
    mask = (1 << kproj) - 1;

    int totalNeighbors = 0;
    if(totalNeighbors < probes)
    {
        // add itself to the list of neighbors
        neighbors[totalNeighbors] = bucket;
        totalNeighbors++;
    }

    uint64_t element = 0;
    for(int k = 0; k < kproj; k++)
    {
       
        uint64_t element = ((1 << (k + 1)) - 1);
        while (totalNeighbors < probes && element <= mask)
        {
            uint64_t temp = (element >> (kproj - 1)) & 1;

            //insert the neighbor
            if(totalNeighbors < probes)
            {
                neighbors[totalNeighbors] = (bucket ^ element) & mask;
                totalNeighbors++;
            }
            else
                return;

            // Gosper's hack to get next combination with same number of bits
            uint64_t c = element & -element;
            uint64_t r = element + c;
            if (r == 0) exit(EXIT_FAILURE); // overflow
            element = (((r ^ element) >> 2) / c) | r;
            // stop if element overflowed beyond 64 bits
            if (element == 0) exit(EXIT_FAILURE);
        }
    }
}

int compare_vectors(const void* a, const void* b, const void* metricContext)
{
    int dimension = *(const int*)metricContext;
    float dist = euclidean_distance(a, b, dimension);
    return (dist == 0.0) ? 0 : (dist < 0.0) ? -1 : 1;
}