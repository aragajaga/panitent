#include "common.h"

#include "window.h"

struct KeyValuePair {
    void* key;
    void* value;
};

struct HashMap {
    struct KeyValuePair* pairs;
    size_t capacity;
    size_t size;
};

void InitHashMap(struct HashMap* map, size_t capacity)
{
    map->pairs = (struct KeyValuePair*)malloc(capacity * sizeof(struct KeyValuePair));
    map->capacity = capacity;
    map->size = 0;
}

void InsertIntoHashMap(struct HashMap* map, void* key, void* value)
{
    if (map->size < map->capacity)
    {
        size_t index = map->size;
        map->pairs[index].key = key;
        map->pairs[index].value = value;
        map->size++;
    }
    else {
    }
}

void* GetFromHashMap(struct HashMap* map, void* key)
{
    for (size_t i = 0; i < map->size; ++i)
    {
        if (map->pairs[i].key == key)
        {
            return map->pairs[i].value;
        }
    }
    return NULL;
}

void RemoveFromHashMapByKey(struct HashMap* map, void* key)
{
    for (size_t i = 0; i < map->size; ++i)
    {
        if (map->pairs[i].key == key)
        {
            // Move the last element to the position being removed
            map->pairs[i] = map->pairs[map->size - 1];
            map->size--;
            return;
        }
    }
}

void RemoveFromHashMapByValue(struct HashMap* map, void* value)
{
    for (size_t i = 0; i < map->size; ++i)
    {
        if (map->pairs[i].value == value)
        {
            // Move the last element to the position being removed
            map->pairs[i] = map->pairs[map->size - 1];
            map->size--;
            return;
        }
    }
}

void FreeHashMap(struct HashMap* map)
{
    free(map->pairs);
    map->capacity = 0;
    map->size = 0;
}

struct HashMap g_hWndMap;

Window* pWndCreating;
