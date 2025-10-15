#ifndef UTILS_H
#define UTILS_H

float uniform_distribution(void* a, void* b);
float gaussian_distribution(void);

void generate_random_vector(float* v, int d);
int dot_product_int(const int* a, const int* b, int d);
float dot_product_float(const float* a, const float* b, int d);

float euclidean_distance(const void* a, const void* b);
int hamming_distance(const int *a, const int *b, int d);

#endif
