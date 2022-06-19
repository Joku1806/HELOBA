#ifndef GENERIC_HASHMAP_H
#define GENERIC_HASHMAP_H

#include "lib/datastructures/generic/generic_vector.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { __GHM_DELETED, __GHM_EMPTY, __GHM_USED } __ghm_hash_state_t;

#define __GHM_SEED1 0x378b0322
#define __GHM_SEED2 0x9fa770ba

uint32_t __ghm_murmur3(const void *key, int len, uint32_t seed);
unsigned __ghm_next_prime(unsigned start);
bool __ghm_should_rehash(unsigned slots_used, unsigned current_size);

#define MAKE_SPECIFIC_HASHMAP_HEADER(K, V, name)                               \
  typedef struct {                                                             \
    K key;                                                                     \
    V value;                                                                   \
    __ghm_hash_state_t state;                                                  \
  } name##_hash_entry_t;                                                       \
                                                                               \
  MAKE_SPECIFIC_VECTOR_HEADER(name##_hash_entry_t, name##_hashentry)           \
                                                                               \
  typedef struct {                                                             \
    name##_hashentry_vector_t *entries;                                        \
    unsigned used_count;                                                       \
  } name##_hashmap_t;                                                          \
                                                                               \
  name##_hashmap_t *name##_hashmap_create();                                   \
  void name##_hashmap_insert(name##_hashmap_t *hm, K key, V value);            \
  void name##_hashmap_delete(name##_hashmap_t *hm, K key);                     \
  bool name##_hashmap_exists(name##_hashmap_t *hm, K key);                     \
  V name##_hashmap_get(name##_hashmap_t *hm, K key);                           \
  void name##_hashmap_destroy(name##_hashmap_t *hm);

#define MAKE_SPECIFIC_HASHMAP_SOURCE(K, V, name, eq)                           \
  MAKE_SPECIFIC_VECTOR_SOURCE(name##_hash_entry_t, name##_hashentry)           \
                                                                               \
  void __##name##_hm_sanity_check(name##_hashmap_t *hm);                       \
  void __##name##_hm_rehash(name##_hashmap_t *hm);                             \
  unsigned __##name##_hm_lookup_for_reading(name##_hashmap_t *hm, K key);      \
  unsigned __##name##_hm_lookup_for_writing(name##_hashmap_t *hm, K key);      \
                                                                               \
  void __##name##_hm_sanity_check(name##_hashmap_t *hm) {                      \
    assert(hm != NULL);                                                        \
  }                                                                            \
                                                                               \
  void __##name##_hm_rehash(name##_hashmap_t *hm) {                            \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    name##_hash_entry_t init = {.state = __GHM_EMPTY};                         \
    name##_hashentry_vector_t *old =                                           \
        name##_hashentry_vector_clone(hm->entries);                            \
                                                                               \
    unsigned size = name##_hashentry_vector_size(hm->entries);                 \
    unsigned new_size = __ghm_next_prime(size + size / 2);                     \
                                                                               \
    name##_hashentry_vector_ensure_capacity(hm->entries, new_size);            \
    name##_hashentry_vector_clear(hm->entries);                                \
    hm->used_count = 0;                                                        \
                                                                               \
    while (!name##_hashentry_vector_full(hm->entries))                         \
      name##_hashentry_vector_append(hm->entries, init);                       \
                                                                               \
    for (unsigned i = 0; i < name##_hashentry_vector_size(old); i++) {         \
      name##_hash_entry_t current = name##_hashentry_vector_at(old, i);        \
                                                                               \
      if (current.state != __GHM_USED)                                         \
        continue;                                                              \
                                                                               \
      name##_hashmap_insert(hm, current.key, current.value);                   \
    }                                                                          \
                                                                               \
    name##_hashentry_vector_destroy(old);                                      \
  }                                                                            \
                                                                               \
  unsigned __##name##_hm_lookup_for_reading(name##_hashmap_t *hm, K key) {     \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    unsigned size = name##_hashentry_vector_size(hm->entries);                 \
    uint32_t initial =                                                         \
        __ghm_murmur3((const void *)&key, sizeof(key), __GHM_SEED1);           \
    uint32_t delta =                                                           \
        __ghm_murmur3((const void *)&key, sizeof(key), __GHM_SEED2);           \
    if (delta == 0)                                                            \
      delta = 1;                                                               \
                                                                               \
    for (unsigned x = 0;; x++) {                                               \
      unsigned hash = (initial + x * delta) % size;                            \
      name##_hash_entry_t current =                                            \
          name##_hashentry_vector_at(hm->entries, hash);                       \
                                                                               \
      if (current.state == __GHM_DELETED ||                                    \
          (current.state == __GHM_USED && !eq(current.key, key)))              \
        continue;                                                              \
                                                                               \
      if (current.state == __GHM_EMPTY)                                        \
        return size;                                                           \
                                                                               \
      return hash;                                                             \
    }                                                                          \
  }                                                                            \
                                                                               \
  unsigned __##name##_hm_lookup_for_writing(name##_hashmap_t *hm, K key) {     \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    unsigned read_index = __##name##_hm_lookup_for_reading(hm, key);           \
    if (read_index < name##_hashentry_vector_size(hm->entries))                \
      return read_index;                                                       \
                                                                               \
    if (__ghm_should_rehash(hm->used_count,                                    \
                            name##_hashentry_vector_size(hm->entries)))        \
      __##name##_hm_rehash(hm);                                                \
                                                                               \
    unsigned size = name##_hashentry_vector_size(hm->entries);                 \
    uint32_t initial =                                                         \
        __ghm_murmur3((const void *)&key, sizeof(key), __GHM_SEED1);           \
    uint32_t delta =                                                           \
        __ghm_murmur3((const void *)&key, sizeof(key), __GHM_SEED2);           \
    if (delta == 0)                                                            \
      delta = 1;                                                               \
                                                                               \
    for (unsigned x = 0;; x++) {                                               \
      unsigned hash = (initial + x * delta) % size;                            \
                                                                               \
      name##_hash_entry_t current =                                            \
          name##_hashentry_vector_at(hm->entries, hash);                       \
      if (current.state == __GHM_EMPTY || current.state == __GHM_DELETED)      \
        return hash;                                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  name##_hashmap_t *name##_hashmap_create() {                                  \
    name##_hashmap_t *hm = malloc(sizeof(name##_hashmap_t));                   \
    assert(hm != NULL);                                                        \
                                                                               \
    hm->entries = name##_hashentry_vector_create();                            \
    hm->used_count = 0;                                                        \
                                                                               \
    __##name##_hm_sanity_check(hm);                                            \
    __##name##_hm_rehash(hm);                                                  \
                                                                               \
    return hm;                                                                 \
  }                                                                            \
                                                                               \
  void name##_hashmap_insert(name##_hashmap_t *hm, K key, V value) {           \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    name##_hash_entry_t added = {                                              \
        .key = key,                                                            \
        .value = value,                                                        \
        .state = __GHM_USED,                                                   \
    };                                                                         \
                                                                               \
    unsigned index = __##name##_hm_lookup_for_writing(hm, key);                \
    name##_hashentry_vector_insert_at(hm->entries, index, added);              \
    hm->used_count++;                                                          \
  }                                                                            \
                                                                               \
  void name##_hashmap_delete(name##_hashmap_t *hm, K key) {                    \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    unsigned index = __##name##_hm_lookup_for_reading(hm, key);                \
    if (index == name##_hashentry_vector_size(hm->entries))                    \
      return;                                                                  \
                                                                               \
    name##_hash_entry_t del = {.state = __GHM_DELETED};                        \
    name##_hashentry_vector_insert_at(hm->entries, index, del);                \
    hm->used_count--;                                                          \
  }                                                                            \
                                                                               \
  bool name##_hashmap_exists(name##_hashmap_t *hm, K key) {                    \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    unsigned index = __##name##_hm_lookup_for_reading(hm, key);                \
    return index < name##_hashentry_vector_size(hm->entries);                  \
  }                                                                            \
                                                                               \
  V name##_hashmap_get(name##_hashmap_t *hm, K key) {                          \
    __##name##_hm_sanity_check(hm);                                            \
                                                                               \
    unsigned index = __##name##_hm_lookup_for_reading(hm, key);                \
    assert(index < name##_hashentry_vector_size(hm->entries));                 \
                                                                               \
    name##_hash_entry_t entry =                                                \
        name##_hashentry_vector_at(hm->entries, index);                        \
    return entry.value;                                                        \
  }                                                                            \
                                                                               \
  void name##_hashmap_destroy(name##_hashmap_t *hm) {                          \
    __##name##_hm_sanity_check(hm);                                            \
    name##_hashentry_vector_destroy(hm->entries);                              \
    free(hm);                                                                  \
  }

#endif