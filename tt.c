#include "tt.h"
#include <string.h>

#define TT_SIZE (1 << 20)   // ~1M entries — power of 2 so masking works

static TTEntry table[TT_SIZE];

void tt_init(void) {
    memset(table, 0, sizeof(table));
}

void tt_clear(void) {
    memset(table, 0, sizeof(table));
}

TTEntry* tt_probe(uint64_t hash) {
    TTEntry* e = &table[hash & (TT_SIZE - 1)];
    if (e->hash == hash && e->depth > 0)   // depth > 0 excludes zeroed/never-written slots
        return e;
    return NULL;
}

void tt_store(uint64_t hash, int depth, int score, TTFlag flag, Move best_move, bool has_move) {
    TTEntry* e = &table[hash & (TT_SIZE - 1)];

    // simplest replacement policy for now: always overwrite.
    // (upgrade later to "keep if existing entry is deeper", once this works)
    e->hash = hash;
    e->depth = depth;
    e->score = score;
    e->flag = flag;
    e->best_move = best_move;
    e->has_move = has_move;
}
