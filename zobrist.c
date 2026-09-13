#include "zobrist.h"

uint64_t zobrist_piece[2][7][64];
uint64_t zobrist_black_to_move;
uint64_t zobrist_castle[4];
uint64_t zobrist_ep_file[8];


// Fixed seed on purpose: for reproductiblit
static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t next_rand(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}


void zobrist_init(void) {
    for (int color = 0; color < 2; color++)
        for (int type = 1; type <= 6; type++)
            for (int sq = 0; sq < 64; sq++)
                zobrist_piece[color][type][sq] = next_rand();

    zobrist_black_to_move = next_rand();

    for (int i = 0; i < 4; i++)
        zobrist_castle[i] = next_rand();

    for (int i = 0; i < 8; i++)
        zobrist_ep_file[i] = next_rand();
}
