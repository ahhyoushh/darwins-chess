#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include "move.h"

#define EMPTY 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define WHITE 1
#define BLACK -1

typedef struct {
    int squares[8][8];
    int side_to_move;

    bool castle_wk, castle_wq, castle_bk, castle_bq;

    int ep_row, ep_col;

    int wk_row, wk_col;
    int bk_row, bk_col;
} Board;

typedef struct {
    int captured_piece;

    int side_to_move;

    bool castle_wk;
    bool castle_wq;
    bool castle_bk;
    bool castle_bq;

    int ep_row;
    int ep_col;

    int wk_row;
    int wk_col;
    int bk_row;
    int bk_col;
} Undo;

void board_init_startpos(Board* b);
bool board_load_fen(Board* b, const char* fen);
void board_print(const Board* b);

void board_make_move(Board* b, Move m, Undo* u);
void board_unmake_move(Board* b, Move m, const Undo* u);
#endif
