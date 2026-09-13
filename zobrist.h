#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>

extern uint64_t zobrist_piece[2][7][64];
extern uint64_t zobrist_black_to_move;
extern uint64_t zobrist_castle[4];
extern uint64_t zobrist_ep_file[8];

void zobrist_init(void);

#endif
