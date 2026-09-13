#ifndef TT_H
#define TT_H

#include <stdint.h>
#include "move.h"
#include <stdbool.h>

typedef enum { TT_EXACT, TT_LOWERBOUND, TT_UPPERBOUND } TTFlag;

typedef struct {
    uint64_t hash;
    int depth;
    int score;
    TTFlag flag;
    Move best_move;
    bool has_move;   // was there a legal move stored (checkmate/stalemate nodes have none)
} TTEntry;

void tt_init(void);
void tt_clear(void);
TTEntry* tt_probe(uint64_t hash);
void tt_store(uint64_t hash, int depth, int score, TTFlag flag, Move best_move, bool has_move);

#endif
