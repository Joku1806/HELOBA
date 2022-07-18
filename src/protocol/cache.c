#include "src/protocol/cache.h"
#include "lib/datastructures/generic/generic_hashmap.h"
#include "lib/datastructures/generic/generic_priority_queue.h"
#include "lib/datastructures/generic/generic_vector.h"
#include "lib/time_util.h"
#include "src/protocol/message.h"
#include <time.h>

// NOTE: Es wäre interessant, die aktuelle Cachegröße abhängig vom Alter des
// letzten Eintrags zu machen. Diese statische Größe sollte aber erstmal
// ausreichen.
static unsigned cache_size = 32;

typedef struct {
  struct timespec timestamp;
  routing_id_t id;
} cache_key_t;

int cache_key_cmp(cache_key_t a, cache_key_t b) {
  if (a.timestamp.tv_sec > b.timestamp.tv_sec) {
    return -1;
  } else if (a.timestamp.tv_sec < b.timestamp.tv_sec) {
    return 1;
  }

  if (a.timestamp.tv_nsec > b.timestamp.tv_nsec) {
    return -1;
  } else if (a.timestamp.tv_nsec < b.timestamp.tv_nsec) {
    return 1;
  }

  return 0;
}

MAKE_SPECIFIC_VECTOR_SOURCE(routing_id_t, rc_key)
MAKE_SPECIFIC_HASHMAP_SOURCE(routing_id_t, cache_entry_t, rc, routing_id_equal)

MAKE_SPECIFIC_PRIORITY_QUEUE_HEADER(cache_key_t, ck)
MAKE_SPECIFIC_VECTOR_SOURCE(cache_key_t, ck)
MAKE_SPECIFIC_PRIORITY_QUEUE_SOURCE(cache_key_t, ck, cache_key_cmp)

// FIXME: besser benennen
rc_hashmap_t *rc_hm;
ck_priority_queue_t *ck_pq;

// TODO: cache_initialize() in main() aufrufen
// FIXME: Tests für den Cache schreiben!
void cache_initialize() {
  rc_hm = rc_hashmap_create();
  ck_pq = ck_priority_queue_create();
}

bool cache_hit(routing_id_t id) { return rc_hashmap_exists(rc_hm, id); }

rc_key_vector_t *cache_contents() { return rc_hashmap_keys(rc_hm); }

cache_hint_t cache_get(routing_id_t id) {
  assert(cache_hit(id));

  cache_hint_t processed;
  memset(&processed, 0, sizeof(processed));

  cache_entry_t raw = rc_hashmap_get(rc_hm, id);

  struct timespec now;
  clock_gettime(CLOCK_BOOTTIME, &now);
  processed.timedelta_us = (now.tv_sec - raw.timestamp.tv_sec) * 1000000 +
                           (now.tv_nsec - raw.timestamp.tv_nsec) / 1000;
  processed.f = raw.f;

  return processed;
}

void cache_remove(routing_id_t id) {
  assert(cache_hit(id));

  for (unsigned i = 0; i < ck_priority_queue_size(ck_pq); i++) {
    cache_key_t current = ck_priority_queue_at(ck_pq, i);

    if (routing_id_equal(id, current.id)) {
      ck_priority_queue_remove_at(ck_pq, i);
      return;
    }
  }

  rc_hashmap_remove(rc_hm, id);
}

void cache_insert(routing_id_t id, frequency_t f) {
  cache_entry_t entry;
  memset(&entry, 0, sizeof(entry));

  entry.id = id;
  entry.f = f;
  // Die Aktionen des anderen Beaglebones sind unabhängig von unserer lokalen
  // Prozesszeit. Um bei den Zeitvergleichen so akkurat wie möglich zu sein,
  // benutzen wir also die Hardware Clock ohne zusätzliche Anpassungen.
  clock_gettime(CLOCK_BOOTTIME, &entry.timestamp);

  if (cache_hit(id)) {
    cache_remove(id);
  } else if (rc_hashmap_size(rc_hm) == cache_size) {
    cache_key_t oldest = ck_priority_queue_peek(ck_pq);
    cache_remove(oldest.id);
  }

  cache_key_t key;
  memset(&key, 0, sizeof(key));

  key.id = entry.id;
  key.timestamp = entry.timestamp;

  ck_priority_queue_push(ck_pq, key);
  rc_hashmap_insert(rc_hm, entry.id, entry);
}