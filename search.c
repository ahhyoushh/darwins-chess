#include "search.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "eval.h"

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>

#define INF        1000000000
#define MATE_SCORE 1000000

static int quiescence(Board* b, int alpha, int beta);

static uint64_t nodes_searched = 0;

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
    MoveList moves;

    generate_legal_moves(b, &moves);
    order_moves(b, &moves);

    if (moves.count == 0) {
	    if (is_in_check(b, b->side_to_move))
	        return -MATE_SCORE;

	    return 0; // STALEMATE
    }

    if (depth == 0)
	    return quiescence(b, alpha, beta);

    int best_score = -INF;

    for (int i = 0; i < moves.count; i++) {
	
	    Move m = moves.moves[i];
	    Undo u;

	    board_make_move(b, m, &u);

	    int score = -negamax(b, depth - 1, -beta, -alpha); // oou shi recursion
	
	    board_unmake_move(b,m, &u);

	    if (score > best_score)
	        best_score = score;

        if (score > alpha)
            alpha = score;

        if (alpha >= beta)
            break;
    }
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

static Move search_at_depth(Board* b, int depth, Move preferred_move, bool has_preferred_move) {
    MoveList moves;
    
    generate_legal_moves(b, &moves);

    if (moves.count == 0) {
        Move no_move = {0};
        return no_move;
    }

    order_moves(b, &moves);

    if (has_preferred_move)
        prioritize_move(&moves, preferred_move);

    Move best_move = moves.moves[0];

    int alpha = -INF;
    int beta = INF;

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        Undo u;

        board_make_move(b, m, &u);

        int score = -negamax(b, depth - 1, -beta, -alpha);

        board_unmake_move(b,m , &u);

        if (score > alpha) {
            alpha = score;
            best_move = m;
        }
    }

    return best_move;
}


Move search_best_move(Board* b, int max_depth) { 
    Move best_move = (Move){0};
    bool has_best_move = false;

    for(int depth = 1; depth <= max_depth; depth++) {
        best_move = search_at_depth(b, depth, best_move, has_best_move);

        has_best_move = true;
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

