#include "board.h"
#include "move.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "zobrist.h"
// #include <assert.h>
//
// static uint64_t compute_hash_from_scratch(const Board* b) {
//     uint64_t h = 0;
//
//     for (int row = 0; row < 8; row++) {
//         for (int col = 0; col < 8; col++) {
//             int piece = b->squares[row][col];
//             if (piece != EMPTY) {
//                 int color = (piece > 0) ? 0 : 1;
//                 int type = abs(piece);
//                 h ^= zobrist_piece[color][type][row * 8 + col];
//             }
//         }
//     }
//
//     if (b->side_to_move == BLACK) h ^= zobrist_black_to_move;
//     if (b->castle_wk) h ^= zobrist_castle[0];
//     if (b->castle_wq) h ^= zobrist_castle[1];
//     if (b->castle_bk) h ^= zobrist_castle[2];
//     if (b->castle_bq) h ^= zobrist_castle[3];
//     if (b->ep_row >= 0) h ^= zobrist_ep_file[b->ep_col];
//
//     return h;
// }
//
// void board_verify_hash(const Board* b) {
//     uint64_t expected = compute_hash_from_scratch(b);
//     if (b->hash != expected) {
//         fprintf(stderr,
//                 "HASH MISMATCH: incremental=%llx expected=%llx\n",
//                 (unsigned long long)b->hash,
//                 (unsigned long long)expected);
//         assert(b->hash == expected);
//     }
// }

static int piece_from_char(char c) {
    int val;
    switch (tolower(c)) {
	case 'p': val = PAWN; break;
	case 'n': val = KNIGHT; break;
	case 'b': val = BISHOP; break;
	case 'r': val = ROOK; break;
	case 'k': val = KING; break;
	case 'q': val = QUEEN; break;
	default: return 0;
    }
    return isupper(c) ? val: -val;
}

static char char_from_piece(int p) {
    char c;
    switch(abs(p)) {
	case PAWN: c ='p'; break;
	case KNIGHT: c = 'n'; break;
	case BISHOP: c = 'b'; break;
	case ROOK: c = 'r'; break;
	case KING: c = 'k'; break;
	case QUEEN: c = 'q'; break;
	default: return '.';
    }
    return p > 0 ? toupper(c) : c;
}

static inline void hash_toggle_piece(Board* b, int piece, int sq) {
    int color = (piece > 0)  ? 0 : 1;
    int type = abs(piece);
    b->hash ^= zobrist_piece[color][type][sq];
}
static bool zobrist_ready = false;

void board_init_startpos(Board* b) {
    if (!zobrist_ready) { zobrist_init(); zobrist_ready = true; }
    board_load_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}


bool board_load_fen(Board* b, const char* fen) {
    if (!zobrist_ready) { zobrist_init(); zobrist_ready = true;}
    memset(b, 0, sizeof(Board));
    b->ep_row = -1;
    b->ep_col = -1;

    int row = 7, col = 0;
    const char* p = fen;

    // 1. Place the peases
    while (*p && *p != ' ') {
        if (*p == '/') {
            row--;
            col = 0;
        } else if (isdigit((unsigned char)*p)) {
            col += (*p - '0');
        } else {
            if (row < 0 || row > 7 || col < 0 || col > 7) return false;
            int piece = piece_from_char(*p);
            b->squares[row][col] = piece;
            if (piece == KING)  { b->wk_row = row; b->wk_col = col; }
            if (piece == -KING) { b->bk_row = row; b->bk_col = col; }
            col++;
        }
        p++;
    }
    if (*p == ' ') p++;

    // 2. Which side to move
    b->side_to_move = (*p == 'w') ? WHITE : BLACK;
    while (*p && *p != ' ') p++;
    if (*p == ' ') p++;

    // 3. castling rights
    while (*p && *p != ' ') {
        if (*p == 'K') b->castle_wk = true;
        if (*p == 'Q') b->castle_wq = true;
        if (*p == 'k') b->castle_bk = true;
        if (*p == 'q') b->castle_bq = true;
        p++;
    }
    if (*p == ' ') p++;

    // 4. en passant target square
    if (*p && *p != '-') {
        b->ep_col = p[0] - 'a';
        b->ep_row = p[1] - '1';
    }

    b->hash = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col ++) {
            int piece = b->squares[row][col];
            if (piece != EMPTY)
                hash_toggle_piece(b, piece, row * 8 + col);
        }
    }
    if (b->side_to_move == BLACK) b->hash ^= zobrist_black_to_move;
    if (b->castle_wk) b->hash ^= zobrist_castle[0];
    if (b->castle_wq) b->hash ^= zobrist_castle[1];
    if (b->castle_bk) b->hash^= zobrist_castle[2];
    if (b->castle_bq) b->hash ^= zobrist_castle[3];
    if (b->ep_col >= 0) b->hash ^= zobrist_ep_file[b->ep_col];
    //TODO: halfmove clock / fullmove number ignored for now

    return true;
}


void board_print(const Board* b) {
    for (int row = 7; row >= 0; row--) {
        printf("%d  ", row + 1);
        for (int col = 0; col < 8; col++) {
            printf("%c ", char_from_piece(b->squares[row][col]));
        }
        printf("\n");
    }
    printf("   a b c d e f g h\n");
    printf("side to move: %s\n", b->side_to_move == WHITE ? "white" : "black");
    printf("castling: %c%c%c%c\n",
           b->castle_wk ? 'K' : '-',
           b->castle_wq ? 'Q' : '-',
           b->castle_bk ? 'k' : '-',
           b->castle_bq ? 'q' : '-');
    if (b->ep_col >= 0)
        printf("en passant: %c%d\n", 'a' + b->ep_col, b->ep_row + 1);
    else
        printf("en passant: -\n");
}
void board_make_move(Board* b, Move m, Undo* u) {

    int piece = b->squares[m.from_row][m.from_col];
    int captured = b->squares[m.to_row][m.to_col];

    u->captured_piece = captured;

    u->side_to_move = b->side_to_move;

    u->castle_wk = b->castle_wk;
    u->castle_wq = b->castle_wq;
    u->castle_bk = b->castle_bk;
    u->castle_bq = b->castle_bq;

    u->ep_row = b->ep_row;
    u->ep_col = b->ep_col;

    u->wk_row = b->wk_row;
    u->wk_col = b->wk_col;
    u->bk_row = b->bk_row;
    u->bk_col = b->bk_col;

    b->ep_row = -1;
    b->ep_col = -1;

    u->hash = b->hash;
    if (b->castle_wk) b->hash ^= zobrist_castle[0];
    if (b->castle_wq) b->hash ^= zobrist_castle[1];
    if (b->castle_bk) b->hash ^= zobrist_castle[2];
    if (b->castle_bq) b->hash ^= zobrist_castle[3];
    if (u->ep_row >= 0) b->hash ^= zobrist_ep_file[u->ep_col];

    if (abs(piece) == KING) {
        if (piece > 0) {
            b->wk_row = m.to_row;
            b->wk_col = m.to_col;

            b->castle_wk = false;
            b->castle_wq = false;
        } else {
            b->bk_row = m.to_row;
            b->bk_col = m.to_col;

            b->castle_bk = false;
            b->castle_bq = false;
        }
    }

    if (abs(piece) == ROOK) {
        if (piece > 0 && m.from_row == 0 && m.from_col == 0)
            b->castle_wq = false;

        if (piece > 0 && m.from_row == 0 && m.from_col == 7)
            b->castle_wk = false;

        if (piece < 0 && m.from_row == 7 && m.from_col == 0)
            b->castle_bq = false;

        if (piece < 0 && m.from_row == 7 && m.from_col == 7)
            b->castle_bk = false;
    }

    if (abs(captured) == ROOK) {
        if (captured > 0 && m.to_row == 0 && m.to_col == 0)
            b->castle_wq = false;

        if (captured > 0 && m.to_row == 0 && m.to_col == 7)
            b->castle_wk = false;

        if (captured < 0 && m.to_row == 7 && m.to_col == 0)
            b->castle_bq = false;

        if (captured < 0 && m.to_row == 7 && m.to_col == 7)
            b->castle_bk = false;
    }

    switch (m.type) {

        case NORMAL:
            hash_toggle_piece(b, piece, m.from_row * 8 + m.from_col);
            if (captured != EMPTY) hash_toggle_piece(b, captured, m.to_row * 8 + m.to_col);
            hash_toggle_piece(b, piece, m.to_row * 8 + m.to_col);


            b->squares[m.from_row][m.from_col] = EMPTY;
            b->squares[m.to_row][m.to_col] = piece;
            break;


        case PROMO: {
            int promoted_piece =
                (piece > 0) ? m.promo_piece : -m.promo_piece;
            hash_toggle_piece(b, piece, m.from_row * 8 + m.from_col);
            if (captured != EMPTY)
                hash_toggle_piece(b, captured, m.to_row * 8 + m.to_col);
            hash_toggle_piece(b, promoted_piece, m.to_row * 8 + m.to_col);

            b->squares[m.from_row][m.from_col] = EMPTY;
            b->squares[m.to_row][m.to_col] = promoted_piece;
            break;
        }


        case EN_PASSANT: {
            int direction = (piece > 0) ? 1 : -1;
            int captured_row = m.to_row - direction;
            int captured_pawn = b->squares[captured_row][m.to_col];

            hash_toggle_piece(b, piece, m.from_row * 8 + m.from_col);
            hash_toggle_piece(b, piece, m.to_row * 8 + m.to_col);
            hash_toggle_piece(b, captured_pawn, captured_row * 8 + m.to_col);

            b->squares[m.from_row][m.from_col] = EMPTY;
            b->squares[m.to_row][m.to_col] = piece;
            b->squares[m.to_row - direction][m.to_col] = EMPTY;
            break;
        }


        case CASTLE_KS:
            if (piece > 0) {


                hash_toggle_piece(b, KING, 4);
                hash_toggle_piece(b, ROOK, 7);
                hash_toggle_piece(b, KING, 6);
                hash_toggle_piece(b, ROOK, 5);

                // White: Ke1 -> g1, Rh1 -> f1
                b->squares[0][4] = EMPTY;
                b->squares[0][7] = EMPTY;

                b->squares[0][6] = KING;
                b->squares[0][5] = ROOK;

                b->wk_row = 0;
                b->wk_col = 6;

                b->castle_wk = false;
                b->castle_wq = false;
            } else {
                hash_toggle_piece(b, -KING, 60);
                hash_toggle_piece(b, -ROOK, 63);
                hash_toggle_piece(b, -KING, 62);
                hash_toggle_piece(b, -ROOK, 61);

                // Black: Ke8 -> g8, Rh8 -> f8
                b->squares[7][4] = EMPTY;
                b->squares[7][7] = EMPTY;

                b->squares[7][6] = -KING;
                b->squares[7][5] = -ROOK;

                b->bk_row = 7;
                b->bk_col = 6;

                b->castle_bk = false;
                b->castle_bq = false;
            }
            break;


        case CASTLE_QS:
            if (piece > 0) {
                hash_toggle_piece(b, KING, 4);
                hash_toggle_piece(b, ROOK, 0);
                hash_toggle_piece(b, KING, 2);
                hash_toggle_piece(b, ROOK, 3);
                    // White: Ke1 -> c1, Ra1 -> d1
                b->squares[0][4] = EMPTY;
                b->squares[0][0] = EMPTY;

                b->squares[0][2] = KING;
                b->squares[0][3] = ROOK;

                b->wk_row = 0;
                b->wk_col = 2;

                b->castle_wk = false;
                b->castle_wq = false;
            } else {
                hash_toggle_piece(b, -KING, 60);
                hash_toggle_piece(b, -ROOK, 56);
                hash_toggle_piece(b, -KING, 58);
                hash_toggle_piece(b, -ROOK, 59);
                // Black: Ke8 -> c8, Ra8 -> d8
                b->squares[7][4] = EMPTY;
                b->squares[7][0] = EMPTY;

                b->squares[7][2] = -KING;
                b->squares[7][3] = -ROOK;

                b->bk_row = 7;
                b->bk_col = 2;

                b->castle_bk = false;
                b->castle_bq = false;
            }
            break;
    }

    if (abs(piece) == PAWN &&
        abs(m.to_row - m.from_row) == 2) {

        b->ep_row = (m.from_row + m.to_row) / 2;
        b->ep_col = m.from_col;
    }

    if (b->castle_wk) b->hash ^= zobrist_castle[0];
    if (b->castle_wq) b->hash ^= zobrist_castle[1];
    if (b->castle_bk) b->hash ^= zobrist_castle[2];
    if (b->castle_bq) b->hash ^= zobrist_castle[3];
    if (b->ep_row >= 0) b->hash ^= zobrist_ep_file[b->ep_col];

    b->hash ^= zobrist_black_to_move;
    b->side_to_move = -b->side_to_move;
}

void board_unmake_move(Board* b, Move m, const Undo* u) {

    switch (m.type) {

        case NORMAL: {
            int piece = b->squares[m.to_row][m.to_col];

            // Move piece back
            b->squares[m.from_row][m.from_col] = piece;

            // Restore captured piece
            b->squares[m.to_row][m.to_col] = u->captured_piece;

            break;
        }


        case PROMO: {
            // Restore the original pawn
            int pawn = (b->squares[m.to_row][m.to_col] > 0)
                     ? PAWN
                     : -PAWN;

            b->squares[m.from_row][m.from_col] = pawn;

            // Restore captured piece, if any
            b->squares[m.to_row][m.to_col] = u->captured_piece;

            break;
        }


        case EN_PASSANT: {
            // Pawn is currently on destination square
            int pawn = b->squares[m.to_row][m.to_col];

            // Move pawn back
            b->squares[m.from_row][m.from_col] = pawn;
            b->squares[m.to_row][m.to_col] = EMPTY;

            // Restore the captured pawn
            int direction = (pawn > 0) ? 1 : -1;

            b->squares[m.to_row - direction][m.to_col] = -pawn;

            break;
        }


        case CASTLE_KS:
            if (u->side_to_move == WHITE) {
                // White: Kg1 -> e1, Rf1 -> h1

                b->squares[0][6] = EMPTY;
                b->squares[0][5] = EMPTY;

                b->squares[0][4] = KING;
                b->squares[0][7] = ROOK;

            } else {
                // Black: Kg8 -> e8, Rf8 -> h8

                b->squares[7][6] = EMPTY;
                b->squares[7][5] = EMPTY;

                b->squares[7][4] = -KING;
                b->squares[7][7] = -ROOK;
            }

            break;


        case CASTLE_QS:
            if (u->side_to_move == WHITE) {
                // White: Kc1 -> e1, Rd1 -> a1

                b->squares[0][2] = EMPTY;
                b->squares[0][3] = EMPTY;

                b->squares[0][4] = KING;
                b->squares[0][0] = ROOK;

            } else {
                // Black: Kc8 -> e8, Rd8 -> a8

                b->squares[7][2] = EMPTY;
                b->squares[7][3] = EMPTY;

                b->squares[7][4] = -KING;
                b->squares[7][0] = -ROOK;
            }

            break;
    }


    /*
     * Restore all state from before the move.
     */

    b->side_to_move = u->side_to_move;

    b->castle_wk = u->castle_wk;
    b->castle_wq = u->castle_wq;
    b->castle_bk = u->castle_bk;
    b->castle_bq = u->castle_bq;

    b->ep_row = u->ep_row;
    b->ep_col = u->ep_col;

    b->wk_row = u->wk_row;
    b->wk_col = u->wk_col;

    b->bk_row = u->bk_row;
    b->bk_col = u->bk_col;

    b->hash = u->hash;
}
