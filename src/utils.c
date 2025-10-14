#include "../include/main.h"

extern int dimension;

// Generates a random number uniformly distributed between a and b 
float uniform_distribution(void* a, void* b)
{
    float a_p = *(float*) a;
    float b_p = *(float*) b;

    return a_p + (b_p - a_p) * ((float)rand() / RAND_MAX);
}

double gaussian_distribution(void)
{
    static int haveSpare = 0;
    static double spare;

    if (haveSpare)
    {
        haveSpare = 0;
        return spare;
    }

    haveSpare = 1;
    double u, v, s;

    do {
        u = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
        v = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
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

double dot_product_double(const void* a, const void* b, int d)
{
    double sum = 0.0;

    float* a_p = (float*)a;
    float* b_p = (float*)b;

    // Debug Reasons
    // they point in the same address
    // printf("%p vs %p\n\n", &(*a_p), &(*a));
    // but casting does not gkriniazei, why?

    for(int i = 0; i < d; i++)
    {
        sum += a_p[i] * b_p[i];
        // printf("%lf, %lf |", (double)a_p[i], (double)b_p[i]);
    }
    // puts("!!!!!!!!!!!!");

    return sum;
}

double euclidean_distance(const void* a, const void* b)
{
    double sum = 0.0;
    double* a_p = (double*)a;
    double* b_p = (double*)b;

    for(int i = 0; i < dimension; i++)
    {
        double diff = a_p[i] - b_p[i];
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

