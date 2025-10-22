#ifndef UTILS_H
#define UTILS_H

#include <math.h>

//given numbers a and b, returns a random float in [a,b)
float uniform_distribution(void* a, void* b);


//returns a random float from a Gaussian distribution with mean 0 and variance 1
float gaussian_distribution(void);

//generates a random vector of dimension d with Gaussian distributed components
void generate_random_vector(float* v, int d);

// normalizes a float vector to unit L2 norm; if zero vector, sets v[0]=1 and others 0
void normalize_vector(float* v, int d);

//computes the dot product of two integer vectors of dimension d
int dot_product_int(const int* a, const int* b, int d);

//computes the dot product of two float vectors of dimension d
float dot_product_float(const float* a, const float* b, int d);

//computes the dot product of a float vector and an int vector (converts int to float) - inlined for performance
static inline float dot_product_float_int(const float* a, const int* b, int d)
{
    float sum = 0.0f;
    for (int i = 0; i < d; i++)
    {
        sum += a[i] * (float)b[i];
    }
    return sum;
}

//computes the Euclidean distance between two points a and b
float euclidean_distance(const void* a, const void* b, const int dimension);

//computes the Euclidean distance between two int vectors (converts to float) - inlined for performance
static inline float euclidean_distance_int(const void* a, const void* b, const int dimension)
{
    float sum = 0.0;
    int* a_p = (int*)a;
    int* b_p = (int*)b;
    for(int i = 0; i < dimension; i++)
    {
        float diff = (float)a_p[i] - (float)b_p[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

//computes the Hamming distance between two binary vectors a and b of dimension d
int hamming_distance(const int *a, const int *b, int d);

//returns the first 'probes' neighbors from point 'a' sorted by hamming distance
void get_hamming_neighbors(const int* a, int probes, int d, int** neighbors);

//comparison function for two vectors a and b
int compare_vectors(const void* a, const void* b, const void* metricContext);

#endif
