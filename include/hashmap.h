#ifndef HASHMAP_H
#define HASHMAP_H
// Simple hashmap implementation for storing pairs of (int, bool)
// Uses open addressing with linear probing for better cache performance

// Forward declaration
typedef struct Hashmap Hashmap;

// initialize a new hashmap by allocating memory for it with given capacity
Hashmap* hashmap_init(int capacity);

//frees all memory used by the hashmap
void hashmap_free(Hashmap* map);

//given a key, returns true if it exists in the hashmap, false otherwise
//should not be used to get the value associated with the key
bool hashmap_search(Hashmap* map, int key);

//inserts a new (key, value) pair into the hashmap
bool hashmap_insert(Hashmap* map, int key, bool value);

//given a key, returns a pointer to the associated value, or NULL if the key does not exist
bool* hashmap_getValue(Hashmap* map, int key);

//removes a (key, value) pair from the hashmap based on the key
bool hashmap_remove(Hashmap* map, int key);

//returns the number of (key, value) pairs in the hashmap
int hashmap_size(Hashmap* map);

//returns true if the hashmap is empty, false otherwise
bool hashmap_is_empty(Hashmap* map);

//prints the contents of the hashmap (for debugging purposes)
void hashmap_print(Hashmap* map);


#endif