// hybrid hash table implementation
// using linked list for collision resolution instead of rehashing
#include "../include/main.h"
extern struct LSH* lsh; // Declare the global LSH variable

// sieve of eratosthenes O(n log (log n))
int nearest_prime(int n)
{
    // initialize all numbers as prime
    int prime[n + 1];
    for (int i = 0; i <= n; i++)
    {
        prime[i] = 1;
    }

    for (int i = 2; i * i <= n; i++)
    {
        // if prime[i] is 1(True) make all multiples of i as 0(False)
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
            return i;
        }
    }
    return 2;
}

struct hash_table
{
    int size;
    int capacity;
    funtion destroy;
    Compare_fun compare;
    Hash_fun hash_function;
    Node *table;
};

HashTable hash_table_create(int capacity, funtion destroy, Compare_fun compare, Hash_fun hash_function)
{
    // initialize the hash table structure
    HashTable hash_table = (HashTable)malloc(sizeof(struct hash_table));
    if (hash_table == NULL)
    {
        return NULL;
    }
    hash_table->size = 0;
    hash_table->capacity = capacity;
    hash_table->destroy = destroy;
    hash_table->compare = compare;
    hash_table->hash_function = hash_function;

    // initialize the table with NULL
    hash_table->table = (Node *)calloc(hash_table->capacity, sizeof(Node));
    return hash_table;
}

int hash_table_insert(HashTable hash_table, void *key, void *data)
{
    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, key);

    // create a new node
    Node new_node = (Node)malloc(sizeof(struct node));
    if (new_node == NULL)
    {
        return -1;
    }
    new_node->data = data;
    new_node->key = key;
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

void *hash_table_search(HashTable hash_table, void *key)
{
    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, key);

    // if the table is empty, return NULL
    if (hash_table->table[hash_value] == NULL)
    {
        return NULL;
    }

    // search for the key in the list
    Node current = hash_table->table[hash_value];
    while (current != NULL)
    {
        if (hash_table->compare(current->key, key) == 0)
        {
            return current->data;
        }
        current = current->next;
    }
    return NULL;
}

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
    // get the hash value
    int hash_value = hash_table->hash_function(hash_table, key);

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
        if (hash_table->compare(current->key, key) == 0)
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

void print_hashtables(const struct LSH* lsh, int dimension)
{
    if (lsh == NULL || lsh->hash_tables == NULL) {
        printf("LSH or hash tables are not initialized.\n");
        return;
    }
    // print the contents of each hash table
    for (int i = 0; i < lsh->L; i++){
        printf("Hash table %d:\n", i);
        for (int j = 0; j < lsh->table_size; j++){
            Node bucket = hash_table_get_bucket(lsh->hash_tables[i], j);
            if (bucket != NULL){
                printf(" Bucket %d: ", j);
                Node current = bucket;
                while (current != NULL ){
                    // Add comprehensive null checks for safety
                    if (current->data == NULL) {
                        printf("(NULL_DATA) -> ");
                    } else {
                        // Verify that the data pointer is accessible
                        float* point = (float*)current->data;
                        // Check if we can safely read the point
                        printf("(");
                        for (int d = 0; d < dimension; d++){
                            printf("%f", point[d]);
                            if (d < dimension - 1){
                                printf(", ");
                            }
                        }
                        printf(") -> ");
                    }
                    
                    // Check if next pointer is accessible before dereferencing
                    if (current->next != NULL) {
                        current = current->next;
                    } else {
                        current = NULL;
                    }
                }
                printf("NULL\n");
            }
            else{
                printf(" Bucket %d: NULL\n", j);
            }
        }
        printf("----------------------------------------------------------------\n");
    }
}