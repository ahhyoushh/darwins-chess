#include "perft.h"
#include "board.h"
#include "movegen.h"
#include <stdio.h>
#include <inttypes.h>

uint64_t perft(Board* b, int depth) {
    // board_verify_hash(b);

    if (depth == 0)
	return 1;

    MoveList moves;
    generate_legal_moves(b, &moves);

    uint64_t nodes = 0;

    for (int i = 0; i < moves.count; i++) {
	Move m = moves.moves[i];
	Undo u;

	board_make_move(b, m, &u);
	// board_verify_hash(b);

	nodes += perft(b, depth - 1);

	board_unmake_move(b, m, &u);
	// board_verify_hash(b);
    }

    return nodes;
}

static void print_move(Move m) {
    printf("%c%d%c%d",
           'a' + m.from_col,
           m.from_row + 1,
           'a' + m.to_col,
           m.to_row + 1);

    if (m.type == PROMO) {
        char promo = '?';

        switch (m.promo_piece) {
            case QUEEN:  promo = 'q'; break;
            case ROOK:   promo = 'r'; break;
            case BISHOP: promo = 'b'; break;
            case KNIGHT: promo = 'n'; break;
        }

        printf("%c", promo);
    }
}

void perft_divide(Board* b, int depth) {
    MoveList moves;
    generate_legal_moves(b, &moves);

    uint64_t total = 0;

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        Undo u;

        board_make_move(b, m, &u);

        uint64_t nodes = perft(b, depth - 1);

        board_unmake_move(b, m, &u);

        print_move(m);
        printf(": %" PRIu64 "\n", nodes);

        total += nodes;
    }

    printf("Total: %" PRIu64 "\n", total);
}
