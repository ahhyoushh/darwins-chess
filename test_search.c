#include <stdio.h>

#include "board.h"
#include "search.h"

static void print_move(Move m) {
    printf("%c%d%c%d",
           'a' + m.from_col,
           m.from_row + 1,
           'a' + m.to_col,
           m.to_row + 1);

    if (m.type == PROMO) {
        switch (m.promo_piece) {
            case QUEEN:  putchar('q'); break;
            case ROOK:   putchar('r'); break;
            case BISHOP: putchar('b'); break;
            case KNIGHT: putchar('n'); break;
        }
    }
}

int main(void) {
    Board b;

    board_init_startpos(&b);

    printf("depth 1 score: %d\n", negamax(&b, 1));
    printf("depth 2 score: %d\n", negamax(&b, 2));
    printf("depth 3 score: %d\n", negamax(&b, 3));

    Move best = search_best_move(&b, 3);

    printf("best move: ");
    print_move(best);
    printf("\n");

    return 0;
}
