#include "../include/main.h"

Hashmap* hashmap_init()
{
    Hashmap* map = (Hashmap*)malloc(sizeof(Hashmap));
    map->pairs = NULL;
    map->size = 0;
    return map; 
}

void hashmap_free(Hashmap* map)
{
    Pair* current = map->pairs;
    while (current != NULL)
    {
        Pair* temp = current;
        current = current->nextPair;
        free(temp);
    }
    free(map);
}

bool hashmap_search(Hashmap* map, int key)
{
    Pair* current = map->pairs;
    while (current != NULL)
    {
        if (current->key == key)
        {
            return true;
        }
        current = current->nextPair;
    }
    return false;
}


bool hashmap_insert(Hashmap* map, int key, bool value)
{
    Pair* current = map->pairs;

    Pair* newPair = (Pair*)malloc(sizeof(Pair));
    if (newPair == NULL)
    {
        return false; // Memory allocation failed
    }
    newPair->key = key;
    newPair->value = value;
    newPair->nextPair = current;
    map->pairs = newPair;
    map->size++;
    return true;
}

bool* hashmap_getValue(Hashmap* map, int key)
{
    Pair* current = map->pairs;
    while (current != NULL)
    {
        if (current->key == key)
        {
            return &current->value;
        }
        current = current->nextPair;
    }
    return NULL;
}

bool hashmap_remove(Hashmap* map, int key)
{
    Pair* current = map->pairs;
    Pair* previous = NULL;
    while (current != NULL)
    {
        if (current->key == key)
        {
            if (previous == NULL)
            {
                map->pairs = current->nextPair;
            }
            else
            {
                previous->nextPair = current->nextPair;
            }
            free(current);
            map->size--;
            return true;
        }
        previous = current;
        current = current->nextPair;
    }
    return false;
}

int hashmap_size(Hashmap* map)
{
    return map->size;
}

bool hashmap_is_empty(Hashmap* map)
{
    return map->size == 0;
}

void hashmap_print(Hashmap* map)
{
    Pair* current = map->pairs;
    while (current != NULL)
    {
        printf("Key: %d, Value: %d\n", current->key, current->value);
        current = current->nextPair;
    }
}