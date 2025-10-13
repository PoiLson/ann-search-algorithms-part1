#include "../include/lsh.h"
#include "../include/utils.h"
#include <math.h>


// create a closure-like function that captures the LSH parameters
int hash_func_impl(const float* p ,const LSH* lsh, int table_index)
{
    int id = 0;
    for (int i = 0; i < lsh->k; i++)
    {
        int func = dot_product(lsh->hash_params[i].v, p, lsh->d);
        func = (int)floor((func + lsh->hash_params[i].t) / lsh->w);
        // combine using linear combination 
        id += func * lsh->linear_combinations[table_index][i] % lsh->num_of_buckets;
    }
    return id % lsh->num_of_buckets % lsh->table_size;
}


// g(p) = ID(p) mod table_size
// where ID(p) = sum(h_i(p) * r_i) mod M
// r_i are random integers saved in the LSH struct
// M is the number of buckets also saved in the LSH struct
// h_i are the hash functions saved in the LSH struct
// g() needs to be a function stored in the LSH struct so it needs to return hash_func
hash_func amplified_hash_function(const LSH* lsh, int table_index)
{
    
    return hash_func_impl;
}
