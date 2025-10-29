#ifndef UTILS_H
#define UTILS_H


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

//computes the dot product of a float vector and a uint8 vector (converts uint8 to float) - inlined for performance
static inline float dot_product_float_uint8(const float* a, const uint8_t* b, int d)
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

//computes the Euclidean distance between two uint8_t vectors (converts to float) - inlined for performance
static inline float euclidean_distance_uint8(const void* a, const void* b, const int dimension)
{
    float sum = 0.0;
    uint8_t* a_p = (uint8_t*)a;
    uint8_t* b_p = (uint8_t*)b;
    for(int i = 0; i < dimension; i++)
    {
        float diff = (float)a_p[i] - (float)b_p[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

// helper: compute Euclidean distance between a uint8_t vector and a float vector
static inline double euclidean_distance_uint8_to_float(const uint8_t* a, const float* b, const int dimension)
{
    double sum = 0.0;
    for (int i = 0; i < dimension; i++) {
        double diff = (double)a[i] - (double)b[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

static inline double euclidean_distance_float_ivfflat(const float* a, const float* b, const int dimension)
{
    double sum = 0.0;

    for(int i = 0; i < dimension; i++)
    {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }

    return sqrt(sum);
}

// unified helper: distance between a dataset point (float or uint8) and a float centroid
static inline double distance_point_to_centroid(const void *point, int data_type, const float *centroid, int d)
{
    if (data_type == DATA_TYPE_FLOAT)
    {
        return euclidean_distance_float_ivfflat((const float *)point, centroid, d);
    }
    else
    {
        return euclidean_distance_uint8_to_float((const uint8_t *)point, centroid, d);
    }
}

//computes the Hamming distance between two binary vectors a and b of dimension d
int hamming_distance(const int *a, const int *b, int d);

//returns the first 'probes' neighbors from point 'a' sorted by hamming distance
void get_hamming_neighbors(uint64_t bucket, int probes, int kproj, uint64_t* neighbors);

//comparison function for two vectors a and b
int compare_vectors(const void* a, const void* b, const void* metricContext);

//casts a vector of given data type to float array
// float* cast_vector_to_float(void* vec, int data_type, int dimension);

inline float* cast_vector_to_float(void* vec, int data_type, int dimension)
{
    float* result = (float*)malloc(dimension * sizeof(float));
    if (!result)
        return NULL;

    if (data_type == DATA_TYPE_FLOAT)
    {
        float* fvec = (float*)vec;
        for (int i = 0; i < dimension; i++)
            result[i] = fvec[i];
    }
    else if (data_type == DATA_TYPE_UINT8)
    {
        uint8_t* u8vec = (uint8_t*)vec;
        for (int i = 0; i < dimension; i++)
            result[i] = (float)u8vec[i];
    }
    else
    {
        free(result);
        return NULL; // unknown data type
    }
    return result;
}
#endif
