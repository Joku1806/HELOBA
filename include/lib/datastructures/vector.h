#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>

typedef struct {
  unsigned capacity;
  unsigned size;
  int *data;
} vector_t;

vector_t *vector_create();
vector_t *vector_create_with_capacity(unsigned capacity);
unsigned vector_size(vector_t *v);
bool vector_at(vector_t *v, unsigned index, int *out);
bool vector_insert(vector_t *v, int item);
bool vector_remove(vector_t *v, unsigned index);
bool vector_destroy(vector_t *v);

#endif