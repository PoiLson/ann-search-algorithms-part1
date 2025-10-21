#include "../include/main.h"

// sieve of eratosthenes O(n log (log n))
static int nearest_prime(int n)
{
    if(n < 2)
        return 2;

    int* prime = (int*)malloc((n + 1) * sizeof(int));
    if (!prime)
        return 2; // Fallback

    for (int i = 0; i <= n; i++)
        prime[i] = 1;

    for (int i = 2; i * i <= n; i++)
    {
        if (prime[i] == 1)
        {
            for (int j = i * i; j <= n; j += i)
            {
                prime[j] = 0;
            }
        }
    }

    // return the nearest prime number
    for (int i = n; i >= 2; i--)
    {
        if (prime[i] == 1)
        {
            free(prime);
            return i;
        }
    }

    free(prime);
    return 2;
}

HashTable hash_table_create(int capacity, int key_size, funtion destroy, Compare_fun compare, Hash_fun hash_function, void* algorithmContext, int table_index, const void* metricContext)
{
    // initialize the hash table structure
    HashTable hash_table = (HashTable)malloc(sizeof(struct hash_table));
    if (hash_table == NULL)
    {
        return NULL;
    }
    hash_table->size = 0;
    hash_table->capacity = capacity;
    hash_table->key_size = key_size;
    hash_table->destroy = destroy;
    hash_table->compare = compare;
    hash_table->hash_function = hash_function;
    hash_table->algorithmContext = algorithmContext;
    hash_table->table_index = table_index;
    hash_table->metricContext = metricContext;

    // initialize the table with NULL
    hash_table->table = (Node *)calloc(hash_table->capacity, sizeof(Node));
    return hash_table;
}

int hash_table_insert(HashTable hash_table, void *key, void *data)
{
    //get the ID value
    int ID = -1;

    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, data, &ID);

    // if ID is not correct, hash_value will also be not correct
    // so no need for further checks
    if(ID == -1)
    {
        perror("ID not configurated!\n");
        exit(EXIT_FAILURE);
    }

    // create a new node
    Node new_node = (Node)malloc(sizeof(struct node));
    if (new_node == NULL)
    {
        return -1;
    }
    new_node->data = data;

    // PROBLEM, TODO
    new_node->key = (void*)malloc(hash_table->key_size);
    memcpy(new_node->key, key, hash_table->key_size);

    new_node->ID = ID;
    new_node->next = NULL;

    // if the table is empty, insert the node
    if (hash_table->table[hash_value] == NULL)
    {
        hash_table->table[hash_value] = new_node;
        hash_table->size++;
        return 0;
    }

    // if the table is not empty, insert the node at the start of the list
    Node current = hash_table->table[hash_value];
    new_node->next = current;
    hash_table->table[hash_value] = new_node;
    hash_table->size++;
    return 0;
}

void* hash_table_search(HashTable hash_table, void *key)
{
    int ID = -1;
    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, key, &ID);

    // if the table is empty, return NULL
    if (hash_table->table[hash_value] == NULL)
    {
        return NULL;
    }

    // search for the key in the list
    Node current = hash_table->table[hash_value];
    while (current != NULL)
    {
        if (hash_table->compare(current->key, key, hash_table->metricContext) == 0)
        {
            return current->data;
        }
        current = current->next;
    }
    return NULL;
}

/*
    Note: field 'context' is owned by the algorithm, not by the hash table.
    The hash table will not free it in hash_table_destroy().
    'context' will be freed from the algorithm's respoective destroy function
*/
void hash_table_destroy(HashTable hash_table)
{
    // destroy the table and the elements in it
    for (int i = 0; i < hash_table->capacity; i++)
    {
        if (hash_table->table[i] != NULL)
        {
            Node current = hash_table->table[i];
            while (current != NULL)
            {
                Node temp = current;
                current = current->next;
                if (hash_table->destroy != NULL)
                {
                    hash_table->destroy(temp->data);
                }
                free(temp->key);
                free(temp);
            }
        }
    }

    free(hash_table->table);
    free(hash_table);
}

int hash_table_size(HashTable hash_table)
{
    return hash_table->size;
}

int hash_table_remove(HashTable hash_table, void *key)
{
    int ID = -1;
    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, key, &ID);

    // if the table is empty, return
    if (hash_table->table[hash_value] == NULL)
    {
        return -1;
    }

    // search for the key in the list
    Node current = hash_table->table[hash_value];
    Node prev = NULL;
    while (current != NULL)
    {
        if (hash_table->compare(current->key, key, hash_table->metricContext) == 0)
        {
            // if the key is found, remove the node
            if (prev == NULL)
            {
                hash_table->table[hash_value] = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            free(current);
            hash_table->size--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -1;
}

int hash_table_capacity(HashTable hash_table)
{
    return hash_table->capacity;
}

Node hash_table_get_bucket(HashTable hash_table, int index)
{
    if (index < 0 || index >= hash_table->capacity)
    {
        return NULL;
    }
    return hash_table->table[index];
}

void print_hashtable(HashTable hash_table, int table_size, int dimension)
{
    if (hash_table == NULL)
    {
        printf("Hash table is not initialized.\n");
        return;
    }

    // Print the contents of the hash table
    for (int j = 0; j < table_size; j++)
    {
        Node bucket = hash_table_get_bucket(hash_table, j);

        if (bucket != NULL)
        {
            printf(" Bucket %d: ", j);

            Node currentBucket = bucket;
            
            while (currentBucket != NULL )
            {

                if (currentBucket->data == NULL)
                    printf("(NULL_DATA) -> ");
                else
                {
                    float* point = (float*)currentBucket->data;

                    // Check if we can safely read the point
                    printf("(");

                    for (int d = 0; d < dimension; d++)
                    {
                        printf("%f", point[d]);

                        if (d < dimension - 1)
                            printf(", ");

                    }

                    printf(") -> ");
                }
                
                // Check if next pointer is accessible before dereferencing
                if (currentBucket->next != NULL)
                    currentBucket = currentBucket->next;
                else
                    currentBucket = NULL;

            }

            printf("NULL\n");
        }
        else
            printf(" Bucket %d: NULL\n", j);

    }

    printf("----------------------------------------------------------------\n");
}

void print_hashtables(int L, int table_size, HashTable* hash_tables, int dimension)
{
    if (hash_tables == NULL)
    {
        printf("Hashtables are not initialized.\n");
        return;
    }

    // Print the contents of each hash table
    for (int i = 0; i < L; i++)
    {
        printf("Hash table %d:\n", i);
        print_hashtable(hash_tables[i], table_size, dimension);

        printf("----------------------------------------------------------------\n");
    }

}