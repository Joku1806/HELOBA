#ifndef ROUTING_H
#define ROUTING_H

#include <stdint.h>

#define MAC_SIZE 6
#define ROUTING_ID_ENCODED_MIN_SIZE 1
#define ROUTING_ID_ENCODED_MAX_SIZE 6

typedef enum {
  everyone,
  leader,
  nonleader,
  specific,
} routing_layer_t;

typedef struct {
  routing_layer_t layer;
  uint8_t optional_MAC[MAC_SIZE];
} routing_id_t;

#endif