#include "lib/datastructures/original/vector.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void __v_sanity_check(vector_t *v);

void __v_sanity_check(vector_t *v) {
  assert(v != NULL);
  assert(v->data != NULL);
  assert(v->size <= v->capacity);
}

void vector_ensure_capacity(vector_t *v, unsigned capacity) {
  __v_sanity_check(v);

  if (capacity < v->capacity)
    return;

  v->capacity = capacity;
  v->data = realloc(v->data, v->capacity * sizeof(int));

  __v_sanity_check(v);
}

vector_t *vector_create() {
  vector_t *vector = malloc(sizeof(vector_t));
  assert(vector != NULL);

  vector->capacity = 0;
  vector->size = 0;
  vector->data = malloc(0);
  assert(vector->data != NULL);

  return vector;
}

vector_t *vector_create_with_capacity(unsigned capacity) {
  vector_t *vector = malloc(sizeof(vector_t));
  assert(vector != NULL);

  vector->capacity = capacity;
  vector->size = 0;
  vector->data = malloc(capacity * sizeof(int));
  assert(vector->data != NULL);

  return vector;
}

unsigned vector_size(vector_t *v) {
  __v_sanity_check(v);
  return v->size;
}

bool vector_empty(vector_t *v) {
  __v_sanity_check(v);
  return v->size == 0;
}

bool vector_full(vector_t *v) {
  __v_sanity_check(v);
  return v->size == v->capacity;
}

int vector_at(vector_t *v, unsigned index) {
  __v_sanity_check(v);
  assert(index < v->size);
  return v->data[index];
}

void vector_insert_at(vector_t *v, unsigned index, int item) {
  __v_sanity_check(v);
  assert(index < v->size);
  v->data[index] = item;
}

void vector_append(vector_t *v, int item) {
  __v_sanity_check(v);

  if (v->size == v->capacity) {
    unsigned cap = v->capacity < 2 ? 2 : v->capacity + v->capacity / 2;
    vector_ensure_capacity(v, cap);
  }

  v->data[v->size++] = item;
}

void vector_swap(vector_t *v, unsigned i1, unsigned i2) {
  __v_sanity_check(v);
  assert(i1 < v->size);
  assert(i2 < v->size);

  int tmp = v->data[i1];
  v->data[i1] = v->data[i2];
  v->data[i2] = tmp;
}

int vector_remove(vector_t *v, unsigned index) {
  __v_sanity_check(v);
  assert(index < v->size);

  int removed = v->data[index];
  v->data[index] = v->data[--v->size];
  return removed;
}

void vector_clear(vector_t *v) {
  __v_sanity_check(v);
  v->size = 0;
}

vector_t *vector_clone(vector_t *v) {
  __v_sanity_check(v);

  vector_t *c = vector_create_with_capacity(v->capacity);
  c->size = v->size;
  memcpy(c->data, v->data, v->size * sizeof(int));
  return c;
}

void vector_destroy(vector_t *v) {
  __v_sanity_check(v);

  free(v->data);
  free(v);
}