#ifndef PERFT_H
#define PERFT_H

#include <stdint.h>
#include "board.h"

uint64_t perft(Board* b, int depth);
void perft_divide(Board* b, int depth);

#endif
