#include "../include/main.h"

int dimension;

Dataset* read_data(const char* dataset_path)
{
    //opens a file with first line the number of points and the dimension
    FILE* file = fopen(dataset_path, "r");
    if (file == NULL){
        fprintf(stderr, "Error opening file: %s\n", dataset_path);
        EXIT_FAILURE;
    }

    Dataset* dataset = (Dataset*)malloc(sizeof(Dataset));
    
    // Read dataset size and dimension
    if (fscanf(file, "%d %d", &(dataset->size), &(dataset->dimension)) != 2)
    {
        fprintf(stderr, "Error reading dataset size and dimension\n");
        fclose(file);

        EXIT_FAILURE;
    }

    dimension = dataset->dimension;
    
    dataset->data = (void**)malloc(dataset->size * sizeof(float*));
    if (dataset->data == NULL)
    {
        fprintf(stderr, "Error allocating memory for dataset\n");
        fclose(file);

        EXIT_FAILURE;
    }
    
    dataset->size = 20;
    for (int i = 0; i < dataset->size; i++)
    {
        dataset->data[i] = (void*)malloc(dataset->dimension * sizeof(float));
        float* data = (float*)dataset->data[i];
        if (data == NULL)
        {
            fprintf(stderr, "Error allocating memory for point %d\n", i);
            fclose(file);

            EXIT_FAILURE;
        }
        
        for (int j = 0; j < dataset->dimension; j++)
        {
            if (fscanf(file, "%f", &(data[j])) != 1)
            {
                fprintf(stderr, "Error reading point %d, coordinate %d\n", i, j);
                fclose(file);

                EXIT_FAILURE;
            }
        }
    }

    fclose(file);
    return dataset;
}

void printPartialDataset(int size, const Dataset* dataset)
{
    for (int i = 0; i < size; i++)
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