#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct node *Node;
typedef struct hash_table *HashTable;

#define MAP_EOF (Node)0
#define MAP_BOF (Node)0

typedef void (*funtion)(void *);

typedef int (*Compare_fun)(void*, void*);

typedef int (*Hash_fun)(HashTable, void*, int*);

struct LSH; 

typedef struct hash_table
{
    int size;
    int capacity;
    funtion destroy;
    Compare_fun compare;
    Hash_fun hash_function;
    Node *table;
}hash_table;

struct node
{
    void *key;
    void *data;
    int ID;
    
    struct node *next;
};

// Finds the prime number before a given number
int find_prime(int number);

// Creates a hash table with a given capacity
HashTable hash_table_create(int capacity, funtion destroy, Compare_fun compare, Hash_fun hash_function);

// Inserts a key and data in the hash table
int hash_table_insert(HashTable hash_table, void *key, void *data);

// Searches for a key in the hash table
void *hash_table_search(HashTable hash_table, void *key);

// Destroys the hash table
void hash_table_destroy(HashTable hash_table);

// Returns the number of elements in the hash table
int hash_table_size(HashTable hash_table);

// Removes the element with the given key from the hash table
int hash_table_remove(HashTable hash_table, void *key);

// Returns the capacity of the hash table
int hash_table_capacity(HashTable hash_table); 

// Returns the head of the linked list at the given index
Node hash_table_get_bucket(HashTable hash_table, int index);

// Print the hashtable given (delete it later if no need TODO)
void print_hashtables2(const struct LSH* lsh, int dimension);

// Print the hashtable given
void print_hashtables(int L, int table_size, HashTable* hash_tables, int dimension);

#endif