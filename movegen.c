#include "movegen.h"
#include "board.h"
#include "move.h"
#include <stdlib.h>


static const int directions[8][2] = {
    {-1,  0},
    { 1,  0},
    { 0, -1},
    { 0,  1},
    {-1, -1},
    {-1,  1},
    { 1, -1},
    { 1,  1}
};

static const int knight_offsets[8][2] = {
    {-2, -1},
    {-2,  1},
    {-1, -2},
    {-1,  2},
    { 1, -2},
    { 1, 2},
    { 2, -1},
    { 2,  1}
};

static const int king_offsets[8][2] = {
    {-1, -1},
    {-1,  0},
    {-1,  1},
    { 0, -1},
    { 0,  1},
    { 1, -1},
    { 1,  0},
    { 1,  1}
};


static void add_move(MoveList* list, int fr, int fc, int tr, int tc, MoveType type, int promo) {
    Move m;
    m.from_row = fr;
    m.from_col = fc;
    m.to_col = tc;
    m.to_row = tr;
    m.type = type;
    m.promo_piece = promo;
    list->moves[list->count] = m;
    list->count++;
}

static bool is_enemy(int piece, int color) {
    return piece != EMPTY && (piece * color) < 0;
}


void generate_pawn_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;

    int direction = (color == WHITE) ? 1 : -1;
    int start_row = (color == WHITE) ? 1 : 6;

    int to_row = row + direction;

    if (to_row < 0 || to_row > 7)
        return;

    // FORWARD MOVES
    if (b->squares[to_row][col] == EMPTY) {

        // PROMOTION
        if (to_row == 0 || to_row == 7) {
            add_move(list, row, col, to_row, col, PROMO, QUEEN);
            add_move(list, row, col, to_row, col, PROMO, ROOK);
            add_move(list, row, col, to_row, col, PROMO, BISHOP);
            add_move(list, row, col, to_row, col, PROMO, KNIGHT);
        } else {
            add_move(list, row, col, to_row, col, NORMAL, 0);
        }

        // DOUBLE MOVE
        if (row == start_row) {
            int to_row2 = row + 2 * direction;

            if (b->squares[to_row2][col] == EMPTY) {
                add_move(list, row, col, to_row2, col, NORMAL, 0);
            }
        }
    }

    // CAPTURES
    int capture_cols[2] = { col - 1, col + 1 };

    for (int i = 0; i < 2; i++) {
        int cc = capture_cols[i];

        if (cc < 0 || cc > 7)
            continue;

        int target = b->squares[to_row][cc];

        if (target != EMPTY && is_enemy(target, color)) {

            // PROMOTION CAPTURE
            if (to_row == 0 || to_row == 7) {
                add_move(list, row, col, to_row, cc, PROMO, QUEEN);
                add_move(list, row, col, to_row, cc, PROMO, ROOK);
                add_move(list, row, col, to_row, cc, PROMO, BISHOP);
                add_move(list, row, col, to_row, cc, PROMO, KNIGHT);
            } else {
                add_move(list, row, col, to_row, cc, NORMAL, 0);
            }
        }
    }
    
    // en passant
    if (b->ep_row >= 0 && b->ep_col >= 0) {
        int to_row = row + direction;

        if (to_row == b->ep_row &&
            abs(col - b->ep_col) == 1) {

            int captured = b->squares[row][b->ep_col];

            if (b->squares[to_row][b->ep_col] == EMPTY &&
                captured == -color * PAWN) {

                add_move(
                    list,
                    row, col,
                    b->ep_row, b->ep_col,
                    EN_PASSANT,
                    0
                );
            }
        }
    }
}


void generate_knight_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;


    // SURPRISE: APPARENTLY WORSE PERFORMANCE (n*n = 25 iterations)
    // for (int dr = -2; dr <= 2; dr++) {
    //     for (int dc = -2; dc <= 2; dc++) {
    //         if (abs(dr) + abs(dc) != 3)
    //             continue;
    //
    //         int to_row = row + dr;
    //         int to_col = col + dc;
    //
    //         if (to_row < 0 || to_row > 7)
    //             continue;
    //         if (to_col < 0 || to_col > 7)
    //             continue;
    //
    //         int target = b->squares[to_row][to_col];
    //
    //         if (target == EMPTY || is_enemy(target, color)) {
    //             add_move(list, row, col, to_row, to_col, NORMAL, 0);
    //         }
    //     }
    // }

    for (int i = 0; i < 8; i++) {
        int to_row = row + knight_offsets[i][0];
        int to_col = col + knight_offsets[i][1];

        if (to_row < 0 || to_row > 7)
            continue;

        if (to_col < 0 || to_col > 7)
            continue;

        int target = b->squares[to_row][to_col];

        if (target == EMPTY || is_enemy(target, color)) {
            add_move(list, row, col, to_row, to_col, NORMAL, 0);
        }
    }
}


void generate_bishop_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;

    for (int i = 4; i < 8; i++) {

        int dr = directions[i][0];
        int dc = directions[i][1];

        int to_row = row + dr;
        int to_col = col + dc;

        while (to_row >= 0 && to_row <= 7 &&
               to_col >= 0 && to_col <= 7) {

            int target = b->squares[to_row][to_col];

            if (target == EMPTY) {
                add_move(list, row, col, to_row, to_col, NORMAL, 0);
            } else {
                if (is_enemy(target, color)) {
                    add_move(list, row, col, to_row, to_col, NORMAL, 0);
                }

                break;
            }

            to_row += dr;
            to_col += dc;
        }
    }
}


void generate_rook_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;

    for (int i = 0; i < 4; i++) {

        int dr = directions[i][0];
        int dc = directions[i][1];

        int to_row = row + dr;
        int to_col = col + dc;

        while (to_row >= 0 && to_row <= 7 &&
               to_col >= 0 && to_col <= 7) {

            int target = b->squares[to_row][to_col];

            if (target == EMPTY) {
                add_move(list, row, col, to_row, to_col, NORMAL, 0);
            } else {
                if (is_enemy(target, color)) {
                    add_move(list, row, col, to_row, to_col, NORMAL, 0);
                }

                break;
            }

            to_row += dr;
            to_col += dc;
        }
    }
}


void generate_queen_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;

    for (int i = 0; i < 8; i++) {

        int dr = directions[i][0];
        int dc = directions[i][1];

        int to_row = row + dr;
        int to_col = col + dc;

        while (to_row >= 0 && to_row <= 7 &&
               to_col >= 0 && to_col <= 7) {

            int target = b->squares[to_row][to_col];

            if (target == EMPTY) {
                add_move(list, row, col, to_row, to_col, NORMAL, 0);
            } else {
                if (is_enemy(target, color)) {
                    add_move(list, row, col, to_row, to_col, NORMAL, 0);
                }

                break;
            }

            to_row += dr;
            to_col += dc;
        }
    }
}

void generate_king_moves(const Board* b, int row, int col, MoveList* list) {
    int piece = b->squares[row][col];
    int color = (piece > 0) ? WHITE : BLACK;

    // NORMAL KING MOVES
    for (int i = 0; i < 8; i++) {
        int to_row = row + king_offsets[i][0];
        int to_col = col + king_offsets[i][1];

        if (to_row < 0 || to_row > 7)
            continue;

        if (to_col < 0 || to_col > 7)
            continue;

        int target = b->squares[to_row][to_col];

        if (target == EMPTY || is_enemy(target, color)) {
            add_move(list, row, col, to_row, to_col, NORMAL, 0);
        }
    }

    // CASTLING
    if (color == WHITE && row == 0 && col == 4) {

        // White kingside: O-O
        if (b->castle_wk &&
            b->squares[0][5] == EMPTY &&   // f1
            b->squares[0][6] == EMPTY &&   // g1
            b->squares[0][7] == ROOK &&    // h1
            !is_square_attacked(b, 0, 4, BLACK) && // e1
            !is_square_attacked(b, 0, 5, BLACK) && // f1
            !is_square_attacked(b, 0, 6, BLACK)) { // g1

            add_move(list, 0, 4, 0, 6, CASTLE_KS, 0);
        }

        // White queenside: O-O-O
        if (b->castle_wq &&
            b->squares[0][1] == EMPTY &&   // b1
            b->squares[0][2] == EMPTY &&   // c1
            b->squares[0][3] == EMPTY &&   // d1
            b->squares[0][0] == ROOK &&    // a1
            !is_square_attacked(b, 0, 4, BLACK) && // e1
            !is_square_attacked(b, 0, 3, BLACK) && // d1
            !is_square_attacked(b, 0, 2, BLACK)) { // c1

            add_move(list, 0, 4, 0, 2, CASTLE_QS, 0);
        }
    }

    if (color == BLACK && row == 7 && col == 4) {

        // Black kingside: O-O
        if (b->castle_bk &&
            b->squares[7][5] == EMPTY &&   // f8
            b->squares[7][6] == EMPTY &&   // g8
            b->squares[7][7] == -ROOK &&   // h8
            !is_square_attacked(b, 7, 4, WHITE) && // e8
            !is_square_attacked(b, 7, 5, WHITE) && // f8
            !is_square_attacked(b, 7, 6, WHITE)) { // g8

            add_move(list, 7, 4, 7, 6, CASTLE_KS, 0);
        }

        // Black queenside: O-O-O
        if (b->castle_bq &&
            b->squares[7][1] == EMPTY &&   // b8
            b->squares[7][2] == EMPTY &&   // c8
            b->squares[7][3] == EMPTY &&   // d8
            b->squares[7][0] == -ROOK &&   // a8
            !is_square_attacked(b, 7, 4, WHITE) && // e8
            !is_square_attacked(b, 7, 3, WHITE) && // d8
            !is_square_attacked(b, 7, 2, WHITE)) { // c8

            add_move(list, 7, 4, 7, 2, CASTLE_QS, 0);
        }
    }
}

void generate_all_moves(const Board* b, MoveList* list) {
    list->count = 0;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {

            int piece = b->squares[row][col];

            if (piece == EMPTY)
                continue;

            if ((piece > 0 && b->side_to_move != WHITE) ||
                (piece < 0 && b->side_to_move != BLACK))
                continue;

            switch (abs(piece)) {

                case PAWN:
                    generate_pawn_moves(b, row, col, list);
                    break;

                case KNIGHT:
                    generate_knight_moves(b, row, col, list);
                    break;

                case BISHOP:
                    generate_bishop_moves(b, row, col, list);
                    break;

                case ROOK:
                    generate_rook_moves(b, row, col, list);
                    break;

                case QUEEN:
                    generate_queen_moves(b, row, col, list);
                    break;

                case KING:
                    generate_king_moves(b, row, col, list);
                    break;
            }
        }
    }
}




bool is_square_attacked(const Board* b, int row, int col, int attacker_color) {

    // PAWNS

    int pawn_row = row - attacker_color;

    if (pawn_row >= 0 && pawn_row <= 7) {

        int left_col = col - 1;
        int right_col = col + 1;

        if (left_col >= 0 &&
            b->squares[pawn_row][left_col] == attacker_color * PAWN) {
            return true;
        }

        if (right_col <= 7 &&
            b->squares[pawn_row][right_col] == attacker_color * PAWN) {
            return true;
        }
    }


    // KNIGHTS

    for (int i = 0; i < 8; i++) {

        int attacker_row = row + knight_offsets[i][0];
        int attacker_col = col + knight_offsets[i][1];

        if (attacker_row < 0 || attacker_row > 7)
            continue;

        if (attacker_col < 0 || attacker_col > 7)
            continue;

        if (b->squares[attacker_row][attacker_col] ==
            attacker_color * KNIGHT) {
            return true;
        }
    }


    // SLIDING PIECES (BISHOP/QUEEN on diagonals, ROOK/QUEEN on straights)

    for (int i = 0; i < 8; i++) {

        int dr = directions[i][0];
        int dc = directions[i][1];

        int attacker_row = row + dr;
        int attacker_col = col + dc;

        while (attacker_row >= 0 && attacker_row <= 7 &&
               attacker_col >= 0 && attacker_col <= 7) {

            int piece = b->squares[attacker_row][attacker_col];

            if (piece != EMPTY) {

                if (i < 4) {
                    // straight directions: rook or queen
                    if (piece == attacker_color * ROOK ||
                        piece == attacker_color * QUEEN) {
                        return true;
                    }
                } else {
                    // diagonal directions: bishop or queen
                    if (piece == attacker_color * BISHOP ||
                        piece == attacker_color * QUEEN) {
                        return true;
                    }
                }

                // Any piece blocks the ray.
                break;
            }

            attacker_row += dr;
            attacker_col += dc;
        }
    }


    // KING

    for (int i = 0; i < 8; i++) {

        int attacker_row = row + king_offsets[i][0];
        int attacker_col = col + king_offsets[i][1];

        if (attacker_row < 0 || attacker_row > 7)
            continue;

        if (attacker_col < 0 || attacker_col > 7)
            continue;

        if (b->squares[attacker_row][attacker_col] ==
            attacker_color * KING) {
            return true;
        }
    }


    return false;
}



bool is_in_check(const Board* b, int color) {

    int king_row;
    int king_col;

    if (color == WHITE) {
        king_row = b->wk_row;
        king_col = b->wk_col;
    } else {
        king_row = b->bk_row;
        king_col = b->bk_col;
    }

    return is_square_attacked(
        b,
        king_row,
        king_col,
        -color
    );
}

void generate_legal_moves(Board* b, MoveList* legal) {
    MoveList psuedo;
    psuedo.count = 0;

    generate_all_moves(b, &psuedo);

    legal->count = 0;

    int side = b->side_to_move;

    for (int i = 0; i < psuedo.count; i++) {
        Move m = psuedo.moves[i];
        Undo u;

        board_make_move(b, m, &u);

        // did this move leave my king in check;
        if(!is_in_check(b, side)) {
            legal->moves[legal->count] = m;
            legal->count++;
        }

        board_unmake_move(b, m, &u);
    }
}
