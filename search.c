#include "search.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "eval.h"
#include "tt.h"

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#define INF        1000000000
#define MATE_SCORE 1000000


static int quiescence(Board* b, int alpha, int beta);
static void prioritize_move(MoveList* moves, Move preferred);

static uint64_t nodes_searched = 0;

/*
 * Time management / stop-polling state. search.c doesn't know it's
 * talking UCI -- it just checks a deadline and calls back into whatever
 * poll function the caller handed it, every POLL_NODE_INTERVAL nodes.
 */
#define POLL_NODE_INTERVAL 2048

static bool           search_aborted   = false;
static bool           has_deadline     = false;
static int64_t        time_limit_ms    = 0;
static struct timespec search_start;
static search_poll_fn active_poll      = NULL;
static uint64_t       next_poll_node   = 0;

static double elapsed_ms_since(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start.tv_sec) * 1000.0 +
           (double)(now.tv_nsec - start.tv_nsec) / 1000000.0;
}

static void check_time_and_poll(void) {
    if (search_aborted)
        return;

    if (nodes_searched < next_poll_node)
        return;

    next_poll_node = nodes_searched + POLL_NODE_INTERVAL;

    if (has_deadline && elapsed_ms_since(search_start) >= (double)time_limit_ms) {
        search_aborted = true;
        return;
    }

    if (active_poll) {
        SearchPollResult r = active_poll();

        if (r == SEARCH_POLL_STOP || r == SEARCH_POLL_QUIT)
            search_aborted = true;
    }
}

static int evaluate_for_side_to_move(const Board* b) {
    int score = evaluate(b);

    if (b->side_to_move == BLACK)
	score = -score;

    return score;
}

static int move_score(const Board* b, Move m) {
    int attacker = material_value(abs(b->squares[m.from_row][m.from_col]));
    int victim = 0;

    if (m.type == EN_PASSANT) {
        victim = 100;
    } else {
        victim = material_value(abs(b->squares[m.to_row][m.to_col]));
    }

    if (victim == 0 && m.type != PROMO)
        return 0;

    int score = victim * 1000 - attacker;

    if (m.type == PROMO)
        score += material_value(m.promo_piece);

    return score;
}


static void order_moves(const Board* b, MoveList* moves) {
    for (int i = 1; i < moves->count; i++) {
        Move key = moves->moves[i];
        int key_score = move_score(b, key);

        int j = i - 1;

        while (j >= 0 &&
               move_score(b, moves->moves[j]) < key_score) {
            moves->moves[j + 1] = moves->moves[j];
            j--;
        }

        moves->moves[j + 1] = key;
    }
}

int negamax(Board* b, int depth, int alpha, int beta) {

    nodes_searched++;

    check_time_and_poll();

    if (search_aborted)
        return 0;

    int alpha_orig = alpha;

    bool has_tt_move = false;
    Move tt_move = (Move){0};

    TTEntry* tte = tt_probe(b->hash);
    if (tte) {
        if (tte->has_move) {
            tt_move = tte->best_move;
            has_tt_move = true;
        }

        if (tte->depth >= depth) {
            if (tte->flag == TT_EXACT) {
                return tte->score;
            } else if (tte->flag == TT_LOWERBOUND) {
                if (tte->score > alpha) alpha = tte->score;
            } else if (tte->flag == TT_UPPERBOUND) {
                if (tte->score < beta) beta = tte->score;
            }
            if (alpha >= beta) {
                return tte->score;
            }
        }
    }

    MoveList moves;
    generate_legal_moves(b, &moves);
    order_moves(b, &moves);

    if (has_tt_move)
        prioritize_move(&moves, tt_move);

    if (moves.count == 0) {
	    if (is_in_check(b, b->side_to_move))
	        return -MATE_SCORE;

	    return 0; // STALEMATE
    }

    if (depth == 0)
	    return quiescence(b, alpha, beta);

    int best_score = -INF;
    Move best_move = moves.moves[0];

    for (int i = 0; i < moves.count; i++) {
	
	    Move m = moves.moves[i];
	    Undo u;

	    board_make_move(b, m, &u);

	    int score = -negamax(b, depth - 1, -beta, -alpha); // oou shi recursion
	
	    board_unmake_move(b,m, &u);

	    if (search_aborted)
	        return 0; // partial result, ancestor discards it anyway

	    if (score > best_score) {
	        best_score = score;
            best_move = m;
        };
        if (score > alpha)
            alpha = score;

        if (alpha >= beta)
            break;
    }

    TTFlag flag;
    if (best_score <= alpha_orig)
        flag = TT_UPPERBOUND;
    else if (best_score >= beta)
        flag = TT_LOWERBOUND;
    else
        flag = TT_EXACT;

    tt_store(b->hash, depth, best_score,flag, best_move, true);
    return best_score;
}

static bool same_move(Move a, Move b) {
    return a.from_row == b.from_row &&
           a.from_col == b.from_col &&
           a.to_row == b.to_row &&
           a.to_col == b.to_col &&
           a.type == b.type &&
           a.promo_piece == b.promo_piece;
}

static void prioritize_move(MoveList* moves, Move preferred) {
    for (int i = 0; i < moves->count; i++) {
        if (same_move(moves->moves[i], preferred)) {
            Move temp = moves->moves[0];
            moves->moves[0] = moves->moves[i];
            moves->moves[i] = temp;

            return;
        }
    }
}

/*
 * Walk the TT forward from b along stored best-moves to build a PV.
 * Re-verifies each move is actually legal in the position it's played
 * from -- the TT has no verification key, just the 64-bit hash, so a
 * collision could otherwise hand back a garbage move here.
 */
static int collect_pv(const Board* b, Move* pv, int max_len) {
    Board scratch = *b;
    int len = 0;

    while (len < max_len) {
        TTEntry* tte = tt_probe(scratch.hash);

        if (!tte || !tte->has_move)
            break;

        MoveList moves;
        generate_legal_moves(&scratch, &moves);

        bool found = false;
        for (int i = 0; i < moves.count; i++) {
            if (same_move(moves.moves[i], tte->best_move)) {
                found = true;
                break;
            }
        }

        if (!found)
            break;

        Undo u;
        board_make_move(&scratch, tte->best_move, &u);

        pv[len++] = tte->best_move;
    }

    return len;
}

static Move search_at_depth(
    Board* b,
    int depth,
    Move preferred_move,
    bool has_preferred_move,
    int* out_score,
    bool* out_aborted
) {
    MoveList moves;
    
    generate_legal_moves(b, &moves);

    *out_aborted = false;

    if (moves.count == 0) {
        Move no_move = {0};
        if (out_score) *out_score = 0;
        return no_move;
    }

    order_moves(b, &moves);

    if (has_preferred_move)
        prioritize_move(&moves, preferred_move);

    TTEntry* tte = tt_probe(b->hash);
    if (tte && tte->has_move)
        prioritize_move(&moves, tte->best_move);

    Move best_move = moves.moves[0];

    int alpha = -INF;
    int beta = INF;

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        Undo u;

        board_make_move(b, m, &u);

        int score = -negamax(b, depth - 1, -beta, -alpha);

        board_unmake_move(b,m , &u);

        if (search_aborted) {
            *out_aborted = true;
            break;
        }

        if (score > alpha) {
            alpha = score;
            best_move = m;
        }
    }

    if (out_score)
        *out_score = alpha;

    return best_move;
}


Move search_best_move(Board* b, int max_depth) { 
    Move best_move = (Move){0};
    bool has_best_move = false;

    /*
     * Reset time/poll state in case search_best_move_timed left it
     * aborted from a previous call -- this entry point never sets a
     * deadline of its own.
     */
    search_aborted = false;
    has_deadline = false;
    active_poll = NULL;
    next_poll_node = POLL_NODE_INTERVAL;

    for(int depth = 1; depth <= max_depth; depth++) {
        int score;
        bool aborted;

        best_move = search_at_depth(b, depth, best_move, has_best_move, &score, &aborted);

        has_best_move = true;
    }

    return best_move;
}

Move search_best_move_timed(
    Board* b,
    SearchLimits limits,
    search_info_fn on_info,
    search_poll_fn poll
) {
    Move best_move = (Move){0};
    bool has_best_move = false;

    search_aborted = false;
    active_poll = poll;
    next_poll_node = POLL_NODE_INTERVAL;

    has_deadline = limits.has_time_limit;
    time_limit_ms = limits.time_limit_ms;
    clock_gettime(CLOCK_MONOTONIC, &search_start);

    int max_depth = limits.max_depth;

    if (max_depth < 1)
        max_depth = 1;
    if (max_depth > MAX_SEARCH_DEPTH)
        max_depth = MAX_SEARCH_DEPTH;

    for (int depth = 1; depth <= max_depth; depth++) {
        int score = 0;
        bool depth_aborted = false;

        Move depth_move = search_at_depth(
            b, depth, best_move, has_best_move, &score, &depth_aborted
        );

        if (depth_aborted) {
            if (!has_best_move) {
                /*
                 * No previously completed depth to fall back on (this
                 * was depth 1 itself getting aborted). search_at_depth
                 * seeds best_move with a legal move before it starts
                 * comparing, so use that instead of leaving best_move
                 * as the zeroed-out sentinel -- we must always hand
                 * back a legal move here, not garbage.
                 */
                best_move = depth_move;
            }
            break; /* discard this depth's partial result otherwise */
        }

        best_move = depth_move;
        has_best_move = true;

        if (on_info) {
            Move pv[MAX_SEARCH_DEPTH];
            int pv_len = collect_pv(b, pv, depth);

            if (pv_len == 0) {
                pv[0] = best_move;
                pv_len = 1;
            }

            uint64_t time_ms = (uint64_t)elapsed_ms_since(search_start);

            on_info(depth, score, nodes_searched, time_ms, pv, pv_len);
        }

        if (search_aborted)
            break;

        /*
         * Simple time-management heuristic, no branching-factor model:
         * each depth usually costs several times the previous one, so
         * if more than half the budget is already gone, don't start a
         * depth we probably can't finish.
         */
        if (has_deadline &&
            elapsed_ms_since(search_start) > (double)time_limit_ms * 0.5) {
            break;
        }
    }

    return best_move;
}

void search_reset_nodes(void) {
    nodes_searched = 0;
}

uint64_t search_get_nodes(void) {
    return nodes_searched;
}


static int quiescence(Board* b, int alpha, int beta) {
    if (search_aborted)
        return 0;

    bool in_check = is_in_check(b, b->side_to_move);

    if (in_check) {
        MoveList moves;
        generate_legal_moves(b, &moves);

        if (moves.count == 0)
            return -MATE_SCORE;

        for (int i = 0; i < moves.count; i++) {
            Move m = moves.moves[i];
            Undo u;


            board_make_move(b,m ,&u);
            int score = -quiescence(b, -beta, -alpha);
            board_unmake_move(b, m, &u);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        return alpha;
    }

    int stand_pat = evaluate_for_side_to_move(b);

    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList moves;
    generate_all_moves(b, &moves);
    order_moves(b, &moves);

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];

        int captured = b->squares[m.to_row][m.to_col];
        bool is_capture = (captured != EMPTY);
        bool is_promotion = (m.type == PROMO);
        bool is_en_passant = (m.type == EN_PASSANT);

        if (!is_capture && !is_promotion && !is_en_passant)
            continue;

        Undo u;
        board_make_move(b, m, &u);

        if (is_in_check(b, -b->side_to_move)) {
            board_unmake_move(b, m, &u);
            continue;
        }
        int score = -quiescence(b, -beta, -alpha);
        board_unmake_move(b, m, &u);

        if (score >= beta) return beta;
        if (score> alpha) alpha = score;

    }
    return alpha;
}

