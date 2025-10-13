#ifndef UTILS_H
#define UTILS_H

#include "main.h"

float uniform_distribution(float a, float b);
float gaussian_distribution(void);
void generate_random_vector(float *v, int d, int gaussian);
float dot_product(const float *a, const float *b, int d);
double euclidean_distance(const float *a, const float *b, int d);
int hamming_distance(const int *a, const int *b, int d);

#endif
