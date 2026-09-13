#include "uci.h"

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Board board;


static char promotion_char(int piece)
{
    switch (piece) {
        case QUEEN:  return 'q';
        case ROOK:   return 'r';
        case BISHOP: return 'b';
        case KNIGHT: return 'n';
        default:     return '\0';
    }
}


static void print_uci_move(Move m)
{
    printf("%c%d%c%d",
           'a' + m.from_col,
           m.from_row + 1,
           'a' + m.to_col,
           m.to_row + 1);

    if (m.type == PROMO)
        putchar(promotion_char(m.promo_piece));
}


static bool parse_uci_move(Board *b, const char *str, Move *out)
{
    size_t len = strlen(str);

    if (len != 4 && len != 5)
        return false;

    if (str[0] < 'a' || str[0] > 'h' ||
        str[2] < 'a' || str[2] > 'h' ||
        str[1] < '1' || str[1] > '8' ||
        str[3] < '1' || str[3] > '8') {
        return false;
    }

    int from_col = str[0] - 'a';
    int from_row = str[1] - '1';
    int to_col   = str[2] - 'a';
    int to_row   = str[3] - '1';

    int promo_piece = 0;

    if (len == 5) {
        switch (tolower((unsigned char)str[4])) {
            case 'q': promo_piece = QUEEN;  break;
            case 'r': promo_piece = ROOK;   break;
            case 'b': promo_piece = BISHOP; break;
            case 'n': promo_piece = KNIGHT; break;
            default:
                return false;
        }
    }

    MoveList moves;
    generate_legal_moves(b, &moves);

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];

        if (m.from_row != from_row ||
            m.from_col != from_col ||
            m.to_row   != to_row ||
            m.to_col   != to_col) {
            continue;
        }

        if (m.type == PROMO) {
            if (promo_piece == m.promo_piece) {
                *out = m;
                return true;
            }

            continue;
        }

        if (promo_piece == 0) {
            *out = m;
            return true;
        }
    }

    return false;
}


static bool apply_uci_move(Board *b, const char *str)
{
    Move m;

    if (!parse_uci_move(b, str, &m)) {
        fprintf(stderr, "invalid move: %s\n", str);
        return false;
    }

    Undo u;
    board_make_move(b, m, &u);

    return true;
}


static bool handle_position(char *line)
{
    char *saveptr = NULL;

    char *token = strtok_r(line, " \t\n", &saveptr);

    if (token == NULL || strcmp(token, "position") != 0)
        return false;

    token = strtok_r(NULL, " \t\n", &saveptr);

    if (token == NULL)
        return false;


    /*
     * position startpos [moves ...]
     */
    if (strcmp(token, "startpos") == 0) {

        board_init_startpos(&board);

        token = strtok_r(NULL, " \t\n", &saveptr);

        if (token == NULL)
            return true;

        if (strcmp(token, "moves") != 0)
            return false;

        while ((token = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
            if (!apply_uci_move(&board, token))
                return false;
        }

        return true;
    }


    /*
     * position fen <6 FEN fields> [moves ...]
     */
    if (strcmp(token, "fen") == 0) {

        char fen[256];
        size_t used = 0;

        fen[0] = '\0';

        for (int i = 0; i < 6; i++) {
            token = strtok_r(NULL, " \t\n", &saveptr);

            if (token == NULL)
                return false;

            int written = snprintf(
                fen + used,
                sizeof(fen) - used,
                "%s%s",
                i == 0 ? "" : " ",
                token
            );

            if (written < 0 ||
                (size_t)written >= sizeof(fen) - used) {
                return false;
            }

            used += (size_t)written;
        }

        if (!board_load_fen(&board, fen))
            return false;

        token = strtok_r(NULL, " \t\n", &saveptr);

        if (token == NULL)
            return true;

        if (strcmp(token, "moves") != 0)
            return false;

        while ((token = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
            if (!apply_uci_move(&board, token))
                return false;
        }

        return true;
    }

    return false;
}


static void handle_go(char *line)
{
    char *saveptr = NULL;

    strtok_r(line, " \t\n", &saveptr); /* "go" */

    char *token = strtok_r(NULL, " \t\n", &saveptr);

    int depth = 4;

    while (token != NULL) {

        if (strcmp(token, "depth") == 0) {
            char *depth_str =
                strtok_r(NULL, " \t\n", &saveptr);

            if (depth_str != NULL)
                depth = atoi(depth_str);
        }

        token = strtok_r(NULL, " \t\n", &saveptr);
    }

    if (depth < 1)
        depth = 1;

    search_reset_nodes();

    Move best = search_best_move(&board, depth);

    printf("bestmove ");
    print_uci_move(best);
    printf("\n");

    fflush(stdout);
}


void uci_loop(void)
{
    board_init_startpos(&board);

    char line[4096];

    while (fgets(line, sizeof(line), stdin) != NULL) {

        if (strncmp(line, "uci", 3) == 0) {

            printf("id name chess-ga\n");
            printf("id author unknown\n");
            printf("uciok\n");
            fflush(stdout);

        } else if (strncmp(line, "isready", 7) == 0) {

            printf("readyok\n");
            fflush(stdout);

        } else if (strncmp(line, "ucinewgame", 10) == 0) {

            board_init_startpos(&board);

        } else if (strncmp(line, "position", 8) == 0) {

            if (!handle_position(line))
                fprintf(stderr, "invalid position command\n");

        } else if (strncmp(line, "go", 2) == 0) {

            handle_go(line);

        } else if (strncmp(line, "quit", 4) == 0) {

            break;

        } else if (strncmp(line, "stop", 4) == 0) {

            /*
             * Search is currently synchronous, so there is
             * nothing to stop yet.
             */

        }
    }
}
