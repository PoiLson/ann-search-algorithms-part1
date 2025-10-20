#include "../include/main.h"


// dimention is globall so that functions in utils can access it easily
int dimension;

// Read a big-endian 32-bit unsigned integer from file
static int read_be_u32(FILE* f, uint32_t* out)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4)
        return -1;
    *out = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
    return 0;
}

// Attempt to read MNIST IDX3 images file format (magic 2051) from an already-open FILE*
static Dataset* read_mnist_idx3_file(FILE* file)
{
    // file is positioned at the start; we've not consumed bytes yet
    long start_pos = ftell(file);
    if (start_pos == -1L) start_pos = 0;

    // Read header: magic, num_images, rows, cols (all BE u32)
    uint32_t magic = 0, num_images = 0, rows = 0, cols = 0;
    if (read_be_u32(file, &magic) != 0)
        exit(EXIT_FAILURE);
    if (magic != 2051) // 0x00000803
        exit(EXIT_FAILURE);
    if (read_be_u32(file, &num_images) != 0) exit(EXIT_FAILURE);
    if (read_be_u32(file, &rows) != 0) exit(EXIT_FAILURE);
    if (read_be_u32(file, &cols) != 0) exit(EXIT_FAILURE);

    if (num_images == 0 || rows == 0 || cols == 0)
        exit(EXIT_FAILURE);

    // Allocate dataset
    Dataset* dataset = (Dataset*)malloc(sizeof(Dataset));
    if (!dataset)
    {
        fprintf(stderr, "Memory allocation failed for MNIST dataset struct\n");
        exit(EXIT_FAILURE);
    }
    dataset->size = (int)num_images;
    dataset->dimension = (int)(rows * cols);
    dimension = dataset->dimension;

    dataset->data = (void**)malloc(dataset->size * sizeof(void*));
    if (!dataset->data)
    {
        fprintf(stderr, "Memory allocation failed for MNIST data pointers\n");
        free(dataset);
        exit(EXIT_FAILURE);
    }

    // Read each image: rows*cols bytes => convert to float [0..255]
    const size_t pixels = (size_t)rows * (size_t)cols;
    unsigned char *buf = (unsigned char*)malloc(pixels);
    if (!buf)
    {
        fprintf(stderr, "Memory allocation failed for MNIST read buffer\n");
        free(dataset->data);
        free(dataset);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < dataset->size; i++)
    {
        if (fread(buf, 1, pixels, file) != pixels)
        {
            fprintf(stderr, "Error reading MNIST image %d\n", i);
            free(buf);
            free(dataset->data);
            free(dataset);
            exit(EXIT_FAILURE);
        }

        float* img = (float*)malloc(dataset->dimension * sizeof(float));
        if (!img)
        {
            fprintf(stderr, "Memory allocation failed for MNIST image %d\n", i);
            free(buf);
            free(dataset->data);
            free(dataset);
            exit(EXIT_FAILURE);
        }
        for (int p = 0; p < dataset->dimension; p++)
        {
            img[p] = (float)buf[p]; // keep 0..255 range; normalization can be applied by caller if desired
        }
        dataset->data[i] = img;
    }

    free(buf);
    return dataset;

}

Dataset* read_data_mnist(const char* images_path)
{
    FILE* file = fopen(images_path, "rb");
    if (!file)
    {
        fprintf(stderr, "Error opening MNIST images file: %s\n", images_path);
        exit(EXIT_FAILURE);
    }
    Dataset* ds = read_mnist_idx3_file(file);
    fclose(file);
    if (!ds)
    {
        fprintf(stderr, "Invalid MNIST images file (magic 2051 expected): %s\n", images_path);
        exit(EXIT_FAILURE);
    }
    return ds;
}

Dataset* read_data(const char* dataset_path)
{
    // Parse as text file with first line: <size> <dimension> then floats
    FILE* file = fopen(dataset_path, "r");
    if (!file)
    {
        fprintf(stderr, "Error reopening file as text: %s\n", dataset_path);
        exit(EXIT_FAILURE);
    }

    Dataset* dataset = (Dataset*)malloc(sizeof(Dataset));
    if (!dataset)
    {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d %d", &(dataset->size), &(dataset->dimension)) != 2)
    {
        fprintf(stderr, "Error reading dataset size and dimension\n");
        fclose(file);
        free(dataset);
        exit(EXIT_FAILURE);
    }

    dimension = dataset->dimension;

    dataset->data = (void**)malloc(dataset->size * sizeof(float*));
    if (dataset->data == NULL)
    {
        fprintf(stderr, "Error allocating memory for dataset\n");
        fclose(file);
        free(dataset);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < dataset->size; i++)
    {
        dataset->data[i] = (void*)malloc(dataset->dimension * sizeof(float));
        float* data = (float*)dataset->data[i];
        if (data == NULL)
        {
            fprintf(stderr, "Error allocating memory for point %d\n", i);
            fclose(file);
            free(dataset->data);
            free(dataset);
            exit(EXIT_FAILURE);
        }
        
        for (int j = 0; j < dataset->dimension; j++)
        {
            if (fscanf(file, "%f", &(data[j])) != 1)
            {
                fprintf(stderr, "Error reading point %d, coordinate %d\n", i, j);
                fclose(file);
                free(dataset->data);
                free(dataset);
                exit(EXIT_FAILURE);
            }
        }
    }

    fclose(file);
    return dataset;
}
 
void printPartialDataset(int size, const Dataset* dataset)
{
    //for the first 'size' points of the dataset, print the coordinates
    for (int i = 0; i < size && i < dataset->size; i++)
    {
        float* row = (float*) dataset->data[i];

        printf("Point %d: ", i);
        for (int j = 0; j < dataset->dimension; j++)
        {
            printf("%f ", row[j]);
        }
        printf("\n");
    }
}

void free_dataset(Dataset* dataset)
{
    if (!dataset) return;
    if (dataset->data)
    {
        for (int i = 0; i < dataset->size; i++)
        {
            free(dataset->data[i]);
        }
        free(dataset->data);
    }
    free(dataset);
}