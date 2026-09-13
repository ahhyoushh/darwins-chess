#ifndef EVAL_H
#define EVAL_H

#include "board.h"

int evaluate(const Board* b);

int material_value(int type); // added for using in move ordering
#endif
