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
        // grow table
        int new_capacity = map->capacity * 2;
        Entry* new_table = calloc(new_capacity, sizeof(Entry));
        if (!new_table) return false;

        // Rehash all existing entries
        for (int i = 0; i < map->capacity; i++) {
            if (map->table[i].occupied) {
                int new_idx = hash_int(map->table[i].key, new_capacity);
                for (int j = 0; j < new_capacity; j++) {
                    int probe = (new_idx + j) % new_capacity;
                    if (!new_table[probe].occupied) {
                        new_table[probe] = map->table[i];
                        break;
                    }
                }
            }
        }

        free(map->table);
        map->table = new_table;
        map->capacity = new_capacity;
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