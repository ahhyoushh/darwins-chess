#ifndef MOVE_H
#define MOVE_H

typedef enum {
    NORMAL,
    PROMO,
    EN_PASSANT,
    CASTLE_KS,
    CASTLE_QS
} MoveType;

typedef struct {
    int from_row;
    int from_col;

    int to_row;
    int to_col;

    MoveType type;

    // Used only when type == PROMO
    int promo_piece;
} Move;

#define MAX_MOVES 256

typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

#endif
