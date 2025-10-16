#ifndef HASHMAP_H
#define HASHMAP_H



typedef struct Pair
{
    int key; //h_i(p)
    bool value; //f's result
    
    struct Pair* nextPair;
} Pair;

typedef struct Hashmap
{
    Pair* pairs;
    int size; //number of Pairs in the table
   
} Hashmap;


Hashmap* hashmap_init();
void hashmap_free(Hashmap* map);
bool hashmap_search(Hashmap* map, int key);
bool hashmap_insert(Hashmap* map, int key, bool value);
bool* hashmap_getValue(Hashmap* map, int key);
bool hashmap_remove(Hashmap* map, int key);
int hashmap_size(Hashmap* map);
bool hashmap_is_empty(Hashmap* map);

void hashmap_print(Hashmap* map);


#endif