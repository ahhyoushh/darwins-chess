#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "move.h"

void generate_pawn_moves(const Board* b, int row, int col, MoveList* list);
void generate_knight_moves(const Board* b, int row, int col, MoveList* list);
void generate_bishop_moves(const Board* b, int row, int col, MoveList* list);
void generate_rook_moves(const Board* b, int row, int col, MoveList* list);
void generate_queen_moves(const Board* b, int row, int col, MoveList* list);
void generate_king_moves(const Board* b, int row, int col, MoveList* list);

void generate_all_moves(const Board* b, MoveList* list); // includes psuedo-legal moves
void generate_legal_moves(Board* b, MoveList* legal); // Remove psuedo legal moves
bool is_square_attacked(const Board* b, int row, int col, int attacker_color);
bool is_in_check(const Board* b, int color);
#endif
