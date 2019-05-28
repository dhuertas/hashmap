#include <string.h>

#include "hashmap.h"

size_t hm_calculate_index(size_t num_buckets, Hash hash) {
  return ((size_t )hash) & (num_buckets-1);
}

//------------------------------------------------------------------------------
Hashmap *hm_create(size_t initialSize,
  Hash (*hash)(Key *key),
  bool (*equals)(Key *a, Key *b)) {

  assert(hash != NULL);
  assert(equals != NULL);

  Hashmap *hm = malloc(sizeof(Hashmap));

  if (hm == NULL) {
    return NULL;
  }

  hm->num_buckets = 1;

  size_t minimumBuckets = initialSize*4/3;

  while (hm->num_buckets <= minimumBuckets) {
    hm->num_buckets <<= 1;
  }

  hm->buckets = calloc(hm->num_buckets, sizeof(Entry*));
  
  if (hm->buckets == NULL) {
    free(hm);
    return NULL;
  }

  hm->size = 0;

  hm->hash = hash;
  hm->equals = equals;

  pthread_mutex_init(&hm->mutex, NULL);

  return hm;
}

//------------------------------------------------------------------------------
void hm_free(Hashmap *hm) {

  size_t i;

  for (i = 0; i < hm->num_buckets; i++) {

    Entry *entry = hm->buckets[i];

    while (entry != NULL) {
      Entry *next = entry->next;
      hm_free_entry(entry);
      entry = next;
    }
  }

  free(hm->buckets);

  pthread_mutex_destroy(&hm->mutex);

  free(hm);
}

//------------------------------------------------------------------------------
Hash hm_hash(Key *key) {

  return hm_murmurhash3_32((uint8_t *)key->value, key->size, 2019);
}

//------------------------------------------------------------------------------
Hash hm_hash_key(Hashmap *hm, Key *key) {

  Hash h = hm->hash(key);

  return h;
}

//------------------------------------------------------------------------------
void *hm_insert(Hashmap *hm, Key *key, void *value) {

  Hash hash = hm_hash_key(hm, key);

  size_t idx = hm_calculate_index(hm->num_buckets, hash);

  Entry **entry = &(hm->buckets[idx]);

  while (true) {
 
    Entry *current = *entry;

    if (current == NULL) {
      *entry = hm_create_entry(key, hash, value);
 
      if (*entry == NULL) {
        return NULL;
      }

      hm->size++;
      hm_expand(hm);

      return NULL;
    }

    if (hm_equal_keys(current->key, current->hash, key, hash, hm->equals)) {

      void *old_value = current->value;
      current->value = value;

      return old_value;
    }

    entry = &(current->next);
  }
}

//------------------------------------------------------------------------------
void *hm_find(Hashmap *hm, Key *key) {

  Hash hash = hm_hash_key(hm, key);
  size_t idx = hm_calculate_index(hm->num_buckets, hash);


  Entry *entry = hm->buckets[idx];
  while (entry != NULL) {

    if (hm_equal_keys(entry->key, entry->hash, key, hash, hm->equals)) {
      return entry->value;
    }

    entry = entry->next;
  }

  return NULL;
}

//------------------------------------------------------------------------------
bool hm_contains(Hashmap *hm, Key *key) {

  Hash hash = hm_hash_key(hm, key);
  size_t idx = hm_calculate_index(hm->num_buckets, hash);

  Entry *entry = hm->buckets[idx];
  while (entry != NULL) {

    if (hm_equal_keys(entry->key, entry->hash, key, hash, hm->equals)) {
      return true;
    }

    entry = entry->next;
  }

  return false;
}

//------------------------------------------------------------------------------
void *hm_remove(Hashmap *hm, Key *key) {

  Hash hash = hm_hash_key(hm, key);
  size_t idx = hm_calculate_index(hm->num_buckets, hash);

  Entry **entry = &(hm->buckets[idx]);
  Entry *current;

  while ((current = *entry) != NULL) {

    if (hm_equal_keys(current->key, current->hash, key, hash, hm->equals)) {
      void *value = current->value;
      *entry = current->next;
      hm_free_entry(current);
      hm->size--;
      return value;
    }

    entry = &current->next;
  }

  return NULL;
}

//------------------------------------------------------------------------------
size_t hm_size(Hashmap *hm) {

  return hm->size;
}

//------------------------------------------------------------------------------
Entry *hm_create_entry(Key *key, Hash hash, void *value) {

  Entry *entry = malloc(sizeof(Entry));

  if (entry == NULL) {
    return NULL;
  }

  // Copy content to entry
  Key *k = malloc(sizeof(Key));

  if (k == NULL) {
    free(entry);
    return NULL;
  }

  k->value = malloc(key->size);

  if (k->value == NULL) {
    free(k);
    free(entry);
    return NULL;
  }

  memcpy(k->value, key->value, key->size);
  k->size = key->size;

  entry->key = k;

  entry->hash = hash;
  entry->value = value;
  entry->next = NULL;

  return entry;
}

//------------------------------------------------------------------------------
void hm_free_entry(Entry *entry) {

  free(entry->key->value);

  entry->key->value = NULL;
  entry->key->size = 0;

  free(entry->key);

  entry->key = NULL;
  entry->value = NULL;
  entry->next = NULL;

  free(entry);
}

//------------------------------------------------------------------------------
bool hm_equal_keys(Key *a, Hash a_hash, Key *b, Hash b_hash, 
  bool (*equals)(Key *, Key *)) {

  if (a == b) {
    return true;
  }

  if (a_hash != b_hash) {
    return false;
  }

  return equals(a, b);
}

//------------------------------------------------------------------------------
void hm_expand(Hashmap *hm) {

  if (hm->size > hm->num_buckets *3/4) {
  
    size_t new_num_buckets = hm->num_buckets << 1;
    Entry **new_buckets = calloc(new_num_buckets, sizeof(Entry*));

    if (new_buckets == NULL) {
      return;
    }

    size_t i;
    for (i = 0; i < hm->num_buckets; i++) {

      Entry *entry = hm->buckets[i];

      while (entry != NULL) {
        Entry *next = entry->next;
        size_t idx = hm_calculate_index(new_num_buckets, entry->hash);
        entry->next = new_buckets[idx];
        new_buckets[idx] = entry;
        entry = next;
      }
    }

    free(hm->buckets);
    hm->buckets = new_buckets;
    hm->num_buckets = new_num_buckets;
  }
}

//------------------------------------------------------------------------------
size_t hm_current_capacity(Hashmap *hm) {

  return hm->num_buckets * 3/4;
}

//------------------------------------------------------------------------------
void hm_lock(Hashmap *hm) {

  pthread_mutex_lock(&hm->mutex);
}

//------------------------------------------------------------------------------
void hm_unlock(Hashmap *hm) {

  pthread_mutex_unlock(&hm->mutex);
}


//------------------------------------------------------------------------------
//
// From Wikipedia article: https://en.wikipedia.org/wiki/MurmurHash
//
uint32_t hm_murmurhash3_32(const uint8_t* key, size_t len, uint32_t seed) {

  uint32_t h = seed;

  if (len > 3) {

    const uint32_t* key_x4 = (const uint32_t*) key;
    size_t i = len >> 2;

    do {

      uint32_t k = *key_x4++;
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
      h = (h << 13) | (h >> 19);
      h = h * 5 + 0xe6546b64;

    } while (--i);

    key = (const uint8_t*) key_x4;
  }

  if (len & 3) {

    size_t i = len & 3;
    uint32_t k = 0;

    do {
      k <<= 8;
      k |= key[i - 1];
    } while (--i);

    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
  }

  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}
