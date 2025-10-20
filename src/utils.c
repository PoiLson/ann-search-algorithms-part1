#include "../include/main.h"

extern int dimension;

// Generates a random number uniformly distributed between a and b 
float uniform_distribution(void* a, void* b)
{
    float a_p = *(float*) a;
    printf("!!!!!!!!!!!!!!!! %f, ", a_p);
    float b_p = *(float*) b;
    printf("%f\n", b_p);

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
    for(int i = 0; i < d; i++)
        v[i] = gaussian_distribution();
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

    // Debug Reasons
    // they point in the same address
    // printf("%p vs %p\n\n", &(*a_p), &(*a));
    // but casting does not gkriniazei, why?

    for(int i = 0; i < d; i++)
    {
        sum += a[i] * b[i];
        // printf("%lf, %lf |", (double)a_p[i], (double)b_p[i]);
    }
    // puts("!!!!!!!!!!!!");

    return sum;
}

float euclidean_distance(const void* a, const void* b)
{
    float sum = 0.0;
    float* a_p = (float*)a;
    float* b_p = (float*)b;

    for(int i = 0; i < dimension; i++)
    {
        float diff = a_p[i] - b_p[i];
        // printf("dim=%d, a[%d]=%f, b[%d]=%f, diff=%f ->", dimension, i, a_p[i], i, b_p[i], diff);
        sum += diff * diff;
    }

    return sqrt(sum);
}

// TODO FOR HUPERCUBE another wrapper of this function
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
void get_hamming_neighbors(const int* a, int probes, int d, int** neighbors)
{
    //it only returns 'probes' neighbors
    int count = 0;
    *neighbors = (int*)malloc(probes * d * sizeof(int));
    if(!(*neighbors))
        return;
    int distance = 0; // (number of bits)
    while (count < probes && distance <= d)
    {
        for(int i = 0; i < (1 << d); i++) // iterate through all possible combinations
        {
            int hamming_dist = 0;
            for(int j = 0; j < d; j++)
            {
                int bit_a = a[j];
                int bit_b = (i >> (d - 1 - j)) & 1; // get j-th bit of i
                if(bit_a != bit_b)
                    hamming_dist++;
            }
            if(hamming_dist == distance)
            {
                // add this neighbor to the list
                for(int k = 0; k < d; k++)
                {
                    (*neighbors)[count * d + k] = (i >> (d - 1 - k)) & 1;
                }
                count++;
                if(count >= probes)
                    break;
            }
        }
        distance++;
    }
}

int compare_vectors(void* a, void* b)
{
    float dist = euclidean_distance(a, b);
    return (dist == 0.0) ? 0 : (dist < 0.0) ? -1 : 1;
}