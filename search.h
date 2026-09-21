#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

#include <stdint.h>
#include <stdbool.h>

#define MAX_SEARCH_DEPTH 64

/* Returned by a search_poll_fn so the search knows whether to keep going.
 * search.c has no idea what UCI is -- the caller (uci.c) decides what
 * "stop"/"quit" mean and hands back one of these. */
typedef enum {
    SEARCH_POLL_CONTINUE,
    SEARCH_POLL_STOP,
    SEARCH_POLL_QUIT
} SearchPollResult;

typedef SearchPollResult (*search_poll_fn)(void);

/* Called once per completed iterative-deepening depth. pv[0..pv_len-1] is
 * the principal variation for that depth (pv[0] == the best move). */
typedef void (*search_info_fn)(
    int depth,
    int score,
    uint64_t nodes,
    uint64_t time_ms,
    const Move* pv,
    int pv_len
);

typedef struct {
    bool    has_time_limit;
    int64_t time_limit_ms;
    int     max_depth;   /* always set; search_best_move_timed clamps it */
} SearchLimits;

int negamax(Board* b, int depth, int alpha, int beta);

Move search_best_move(Board* b, int depth);

Move search_best_move_timed(
    Board* b,
    SearchLimits limits,
    search_info_fn on_info,
    search_poll_fn poll
);

void search_reset_nodes(void);

uint64_t search_get_nodes(void);

#endif
