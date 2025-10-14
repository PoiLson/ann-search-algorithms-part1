#include "../include/main.h"

extern int dimension;

// Generates a random number uniformly distributed between a and b 
float uniform_distribution(float a, float b)
{
    return a + (b - a) * ((float)rand() / RAND_MAX);
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
        u = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
        v = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);

    s = sqrtf(-2.0f * logf(s) / s);
    spare = v * s;

    return u * s;
}

void generate_random_vector(float* v, int d)
{
    for(int i = 0; i < d; i++)
        v[i] = gaussian_distribution();
}

// TODO make it void pointer in the future using the real datasets
float dot_product(const float* a, const float* b, int d)
{
    float sum = 0.0f;

    for(int i = 0; i < d; i++)
        sum += a[i] * b[i];

    return sum;
}

double euclidean_distance(const float* a, const float* b)
{
    double sum = 0.0;

    for(int i = 0; i < dimension; i++)
    {
        double diff = (double)a[i] - (double)b[i];
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

