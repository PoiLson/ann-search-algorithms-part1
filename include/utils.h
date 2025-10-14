#ifndef UTILS_H
#define UTILS_H

float uniform_distribution(void* a, void* b);
double gaussian_distribution(void);

void generate_random_vector(float* v, int d);
int dot_product_int(const int* a, const int* b, int d);
double dot_product_double(const void* a, const void* b, int d);

double euclidean_distance(const void* a, const void* b);
int hamming_distance(const int *a, const int *b, int d);

#endif
