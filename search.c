#include "search.h"

#include "board.h"
#include "movegen.h"
#include "eval.h"
#include <stdint.h>
#include <sys/types.h>

#define INF        1000000000
#define MATE_SCORE 1000000

static uint64_t nodes_searched = 0;

static int evaluate_for_side_to_move(const Board* b) {
    int score = evaluate(b);

    if (b->side_to_move == BLACK)
	score = -score;

    return score;
}


int negamax(Board* b, int depth, int alpha, int beta) {

    nodes_searched++;
    MoveList moves;

    generate_legal_moves(b, &moves);

    if (moves.count == 0) {
	    if (is_in_check(b, b->side_to_move))
	        return -MATE_SCORE;

	    return 0; // STALEMATE
    }

    if (depth == 0)
	    return evaluate_for_side_to_move(b);

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

Move search_best_move(Board* b, int depth) {
    MoveList moves;
    generate_legal_moves(b, &moves);

    if (moves.count == 0) {
	Move no_move = {0};
	return no_move;
    }

    Move best_move = moves.moves[0];

    int alpha = -INF;
    int beta= INF;

    for (int i = 0; i < moves.count; i++) {
	Move m = moves.moves[i];
	Undo u;

	board_make_move(b, m, &u);

	int score = -negamax(b, depth - 1, -beta, -alpha);

	board_unmake_move(b, m, &u);

	if (score > alpha) {
	    alpha = score;
	    best_move = m;
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
