#include "../include/main.h"

typedef struct {
    int key;
    bool value;
    bool occupied;
} Entry;

struct Hashmap {
    Entry* table;
    int capacity;
    int size;
};

static inline int hash_int(int key, int capacity) {
    // simple multiplicative hash
    unsigned int h = (unsigned int)key * 2654435761u;
    return (int)(h % capacity);
}

Hashmap* hashmap_init(int capacity) {
    Hashmap* map = malloc(sizeof(Hashmap));
    map->capacity = capacity;
    map->size = 0;
    map->table = calloc(capacity, sizeof(Entry));
    return map;
}

void hashmap_free(Hashmap* map) {
    free(map->table);
    free(map);
}

bool* hashmap_getValue(Hashmap* map, int key) {
    int idx = hash_int(key, map->capacity);
    for (int i = 0; i < map->capacity; i++) {
        int probe = (idx + i) % map->capacity;
        if (!map->table[probe].occupied) return NULL;
        if (map->table[probe].key == key) return &map->table[probe].value;
    }
    return NULL;
}

bool hashmap_insert(Hashmap* map, int key, bool value) {
    if (map->size * 2 >= map->capacity) {
        // TODO: grow table if needed
        return false;
    }
    int idx = hash_int(key, map->capacity);
    for (int i = 0; i < map->capacity; i++) {
        int probe = (idx + i) % map->capacity;
        if (!map->table[probe].occupied) {
            map->table[probe].key = key;
            map->table[probe].value = value;
            map->table[probe].occupied = true;
            map->size++;
            return true;
        }
        if (map->table[probe].key == key) {
            map->table[probe].value = value;
            return true;
        }
    }
    return false;
}