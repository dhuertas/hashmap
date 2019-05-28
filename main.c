#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "hashmap.h"

#define EXIT_OK  0
#define EXIT_ERR 1

bool my_equal(Key *a, Key *b) {

  if (a->size != b->size) return false;

  int res = memcmp(a->value, b->value, a->size);

  return res == 0 ? true : false;
}

int main(int argc, char *argv[]) {

  Key keys[] = {
    { "a", 1 },
    { "b", 1 },
    { "c", 1 },
    { "d", 1 },
    { "e", 1 },
    { "f", 1 },
    { "g", 1 },
    { "h", 1 },
    { "i", 1 },
    { "j", 1 },
    { "k", 1 },
    { "l", 1 }
  };

  char *words[] = {
    "slip",
    "secretary",
    "abortive",
    "difficult",
    "lettuce",
    "finicky",
    "puncture",
    "play",
    "handsome",
    "cuddly",
    "gleaming",
    "trip"
  };

  // hashmap
  Hashmap *map = hm_create(16, &hm_hash, &my_equal);

  hm_lock(map);

  int i;
  for (i = 0; i < 12; i++) {
    Key *k = &keys[i];
    hm_insert(map, k, words[i]);

    printf("added key(%s, %lu) value(%s) to map(%u)\n", 
      k->value, k->size, words[i], hm_size(map));
  }

  Key f = { "d", 1 };

  printf("search key(%s, %lu)\n", f.value, f.size);

  char *res = (char *)hm_find(map, &f);

  hm_unlock(map);

  printf("res(%s)\n", res);

  hm_free(map);

  return EXIT_OK;
}
