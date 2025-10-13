#include "../include/lsh.h"

// Helper Functions For LSH Algorithm

// HYPERCUBE IN NEED OF IT
static inline int h_function(const LSH_hash_function* h, const float* p, int d, double w)
{
    float dot = dot_product(h->v, p, d);
    return (int)floor((dot + h->t) / w);
}

