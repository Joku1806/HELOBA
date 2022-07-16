#ifndef SWAP_H
#define SWAP_H

#include "src/protocol/message.h"
#include <stdbool.h>

typedef bool (*handler_f)(message_t *msg);
void register_automatic_swap_handlers();

bool perform_swap(frequency_t with);
bool handle_do_swap(message_t *);
bool handle_do_migrate(message_t *);

#endif
