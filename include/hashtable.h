#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
// hybrid hash table implementation
// the way it handles collisions is by chaining with linked lists, so it has fixed capacity

//forward declarations
struct LSH; 

typedef struct node *Node; // legacy type (no longer used for traversal)
typedef struct hash_table *HashTable;

#define MAP_EOF (Node)0
#define MAP_BOF (Node)0

// Function pointer types
typedef void (*funtion)(void *);
typedef int (*Compare_fun)(const void*, const void*, const void*);
typedef int (*Hash_fun)(HashTable, void*, uint64_t*);


// Entry stored in a bucket (array-based buckets)
typedef struct HTEntry {
    void* key;      // key (e.g., dataset index)
    void* data;     // pointer to vector
    uint64_t ID;    // full 64-bit amplified ID (optional filter)
} HTEntry;

// Internal bucket representation: dynamic array of entries
typedef struct HTBucket {
    HTEntry* items;
    int count;
    int capacity;
} HTBucket;

typedef struct hash_table
{
    int size; // number of elements in the table
    int capacity; // number of buckets
    int key_size; // size of the key in bytes eg sizeof(int)
    funtion destroy; // function to destroy data
    Compare_fun compare; // function to compare keys
    Hash_fun hash_function; // function to hash data to buckets

    // Optional context so hash functions can avoid globals
    void* algorithmContext;        // pointer to algorithm context data (e.g., LSH*, Hypercube*), Algorithm context for hashing
    int table_index;      // index of this table in a family (e.g., 0 to L-1 for LSH)

    // DID NOT DELETED IT, SO WE DO NOT HAVE TO CHANGE THE PROTOTYPE OF SEARCH AND REMOVE
    const void* metricContext; // this will be the dimension pointer or any metric context

    // Array of buckets implemented as dynamic arrays (cache-friendly)
    HTBucket* buckets;
}hash_table;


// Legacy node structure kept for backward compatibility with old APIs (unused now)
struct node { void* key; void* data; uint64_t ID; struct node *next; };

// Finds the prime number before a given number
int nearest_prime(int n);

// Creates a hash table with a given capacity/ nummber of buckets
HashTable hash_table_create(int capacity, int key_size, funtion destroy, Compare_fun compare, Hash_fun hash_function, void* algorithmContext, int table_index, const void* metricContext);

// Inserts a key and data in the hash table
int hash_table_insert(HashTable hash_table, void *key, void *data);

// Searches for a key in the hash table and returns the associated data
void *hash_table_search(HashTable hash_table, void *key);

// Destroys the hash table
void hash_table_destroy(HashTable hash_table);

// Returns the number of elements in the hash table
int hash_table_size(HashTable hash_table);

// Removes the element with the given key from the hash table
int hash_table_remove(HashTable hash_table, void *key);

// Returns the capacity of the hash table
int hash_table_capacity(HashTable hash_table); 

// Returns a pointer to the contiguous entries array for a bucket and sets out_count
// The returned pointer is owned by the table; do not free it.
const HTEntry* hash_table_get_bucket_entries(HashTable hash_table, int index, int* out_count);

// Deprecated: linked-list API (returns NULL)
Node hash_table_get_bucket(HashTable hash_table, int index);

// Print the hashtable given
void print_hashtable(HashTable hash_table, int table_size, int dimension);

// Print all hashtables
void print_hashtables(int L, int table_size, HashTable* hash_tables, int dimension);

// Helpers to retrieve hash table algorithm context and index
static inline void* hash_table_get_algorithm_context(HashTable ht)
{
    return ((struct hash_table*)ht)->algorithmContext;
}

static inline int hash_table_get_index(HashTable ht)
{
    return ((struct hash_table*)ht)->table_index;
}

#endif