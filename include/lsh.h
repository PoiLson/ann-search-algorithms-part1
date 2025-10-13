#ifndef LSH_H
#define LSH_H

#include "main.h"

// Data structure for the hash function h(p)
typedef struct 
{
    float *v; // projection vector
    float t;
}LSH_hash_function;

typedef struct 
{
    int* r;
    int m;
}LSH_ID;

typedef struct LSH_hash_function LSH_hash_function;

// Data Structure for the full LSH
typedef struct
{
    int d; 
    int L; 
    int k; 
    double w; 
    
    int (*h_hash_ptr)(const LSH_hash_function* h, const float* p, int d, double w);

} LSH;

#endif