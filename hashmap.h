#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>

typedef uint32_t Hash;

typedef struct Key Key;
struct Key {
  void *value;
  size_t size;
};

typedef struct Entry Entry;
struct Entry { 
  Key *key;
  Hash hash;
  void *value;
  Entry *next;
};
 
typedef struct Hashmap Hashmap;
struct Hashmap {
  Entry **buckets;
  size_t num_buckets;
  Hash (*hash)(Key *key);
  bool (*equals)(Key *a, Key *b);
  pthread_mutex_t mutex;
  size_t size;
};

Hashmap *hm_create(size_t initialSize, 
  Hash (*hash)(Key *key), 
  bool (*equals)(Key *a, Key *b));

void hm_free(Hashmap *hm);

Hash hm_hash(Key *key);

Hash hm_hash_key(Hashmap *hm, Key *key);

void *hm_insert(Hashmap *hm, Key *key, void *value);

void *hm_find(Hashmap *hm, Key *key);

bool hm_contains(Hashmap *hm, Key *key);

void *hm_remove(Hashmap *hm, Key *key);

size_t hm_size(Hashmap *hm);

size_t hm_calculate_index(size_t num_buckets, Hash hash);

Entry *hm_create_entry(Key *key, Hash hash, void *value);

void hm_free_entry(Entry *entry);

bool hm_equal_keys(Key *a, Hash a_hash, Key *b, Hash b_hash, bool (*equals)(Key *a, Key *b));

void hm_expand(Hashmap *hm);

size_t hm_current_capacity(Hashmap *hm);

void hm_lock(Hashmap *hm);

void hm_unlock(Hashmap *hm);

uint32_t hm_murmurhash3_32(const uint8_t* key, size_t len, uint32_t seed);

#endif
