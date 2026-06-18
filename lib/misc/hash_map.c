#include "hash_map.h"

#include <stdlib.h>
#include <string.h>

unsigned long djb2_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % HASH_MAP_SIZE;
}

void hash_map_set(HashMap *map, const char *key, const char *value)
{
    unsigned long idx = djb2_hash(key);
    HashNode *curr = map->buckets[idx];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            free(curr->value);
            curr->value = strdup(value);
            return;
        }
        curr = curr->next;
    }

    HashNode *new_node = malloc(sizeof(HashNode));
    new_node->key = strdup(key);
    new_node->value = strdup(value);
    new_node->next = map->buckets[idx];
    map->buckets[idx] = new_node;
}

const char *hash_map_get(HashMap *map, const char *key)
{
    unsigned long idx = djb2_hash(key);
    HashNode *curr = map->buckets[idx];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }

    return NULL;
}
