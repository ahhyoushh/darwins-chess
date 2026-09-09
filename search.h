#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

#include <stdint.h>

int negamax(Board* b, int depth, int alpha, int beta);

Move search_best_move(Board* b, int depth);

void search_reset_nodes(void);

uint64_t search_get_nodes(void);

#endif
