#ifndef HASH_MAP_H
#define HASH_MAP_H

#define HASH_MAP_SIZE 1024

typedef struct HashNode {
    char *key;
    char *value;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_MAP_SIZE];
} HashMap;

/* Compute the bucket index for a cache key using djb2. */
unsigned long djb2_hash(const char *str);

/* Insert or replace a key/value pair in the hash map. */
void hash_map_set(HashMap *map, const char *key, const char *value);

/* Return the stored value for a key, or NULL when the key is absent. */
const char *hash_map_get(HashMap *map, const char *key);

#endif
