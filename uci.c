#include "uci.h"

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

static Board board;

/* Set by uci_poll_stdin() when it consumes a "quit" line during search,
 * since that line never makes it back to uci_loop()'s own fgets(). */
static bool quit_requested = false;


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


/*
 * Polled by search.c every POLL_NODE_INTERVAL nodes during a timed
 * search. Non-blocking: selects on stdin with a zero timeout (POSIX
 * select(), or WaitForSingleObject on Windows) so we only know whether
 * input is ready and never stall the search waiting on input that never
 * comes.
 *
 * Relies on uci_loop() having put stdin in unbuffered mode -- otherwise
 * stdio can silently buffer a "stop" ahead of us and the poll won't see
 * it as ready.
 */
static SearchPollResult uci_poll_stdin(void)
{
#ifndef _WIN32
    fd_set fds;
    struct timeval tv = {0, 0};

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

    if (ready <= 0)
        return SEARCH_POLL_CONTINUE;
#else
    /*
     * GetStdHandle(STD_INPUT_HANDLE) works both for console input
     * (signaled on key events) and for the pipe a UCI GUI uses: an
     * anonymous-pipe read handle is signaled whenever data has been
     * written to it.
     */
    if (WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 0) != WAIT_OBJECT_0)
        return SEARCH_POLL_CONTINUE;
#endif

    char line[4096];

    if (fgets(line, sizeof(line), stdin) == NULL)
        return SEARCH_POLL_CONTINUE;

    if (strncmp(line, "stop", 4) == 0)
        return SEARCH_POLL_STOP;

    if (strncmp(line, "quit", 4) == 0) {
        quit_requested = true;
        return SEARCH_POLL_QUIT;
    }

    if (strncmp(line, "isready", 7) == 0) {
        printf("readyok\n");
        fflush(stdout);
    }

    /* Anything else arriving mid-search is ignored. */
    return SEARCH_POLL_CONTINUE;
}


static void uci_on_search_info(
    int depth,
    int score,
    uint64_t nodes,
    uint64_t time_ms,
    const Move *pv,
    int pv_len)
{
    double seconds = (double)time_ms / 1000.0;
    double nps = (seconds > 0.0) ? (double)nodes / seconds : 0.0;

    printf("info depth %d score cp %d nodes %" PRIu64
           " nps %.0f time %" PRIu64 " pv",
           depth, score, nodes, nps, time_ms);

    for (int i = 0; i < pv_len; i++) {
        putchar(' ');
        print_uci_move(pv[i]);
    }

    putchar('\n');
    fflush(stdout);
}


static void handle_go(char *line)
{
    char *saveptr = NULL;

    strtok_r(line, " \t\n", &saveptr); /* "go" */

    char *token = strtok_r(NULL, " \t\n", &saveptr);

    bool have_depth = false;
    int depth = 0;

    bool have_movetime = false;
    long movetime = 0;

    bool have_wtime = false, have_btime = false;
    long wtime = 0, btime = 0;

    long winc = 0, binc = 0;

    bool infinite = false;

    while (token != NULL) {

        if (strcmp(token, "depth") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL) {
                depth = atoi(v);
                have_depth = true;
            }

        } else if (strcmp(token, "movetime") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL) {
                movetime = atol(v);
                have_movetime = true;
            }

        } else if (strcmp(token, "wtime") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL) {
                wtime = atol(v);
                have_wtime = true;
            }

        } else if (strcmp(token, "btime") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL) {
                btime = atol(v);
                have_btime = true;
            }

        } else if (strcmp(token, "winc") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL)
                winc = atol(v);

        } else if (strcmp(token, "binc") == 0) {
            char *v = strtok_r(NULL, " \t\n", &saveptr);

            if (v != NULL)
                binc = atol(v);

        } else if (strcmp(token, "infinite") == 0) {
            infinite = true;
        }

        token = strtok_r(NULL, " \t\n", &saveptr);
    }

    SearchLimits limits = {0};

    if (infinite) {

        limits.has_time_limit = false;
        limits.max_depth = MAX_SEARCH_DEPTH;

    } else if (have_movetime) {

        limits.has_time_limit = true;
        limits.time_limit_ms = movetime;
        limits.max_depth = have_depth ? depth : MAX_SEARCH_DEPTH;

    } else if (have_wtime || have_btime) {

        long time_left = (board.side_to_move == WHITE) ? wtime : btime;
        long increment  = (board.side_to_move == WHITE) ? winc  : binc;

        /*
         * Simple time allocation: a fixed fraction of what's left, plus
         * the increment. No move-count/phase awareness -- good enough
         * until there's a reason to make it smarter.
         */
        long budget = time_left / 30 + increment / 2;

        long safety_margin = 50; /* leave slack for I/O, GUI overhead */

        if (budget > time_left - safety_margin)
            budget = time_left - safety_margin;

        if (budget < 50)
            budget = 50;

        limits.has_time_limit = true;
        limits.time_limit_ms = budget;
        limits.max_depth = have_depth ? depth : MAX_SEARCH_DEPTH;

    } else if (have_depth) {

        limits.has_time_limit = false;
        limits.max_depth = depth;

    } else {

        /*
         * Bare "go" with nothing else specified. Not strictly UCI (the
         * spec treats this as infinite), but keeps manual testing by
         * typing commands into stdin convenient.
         */
        limits.has_time_limit = false;
        limits.max_depth = 4;
    }

    if (limits.max_depth < 1)
        limits.max_depth = 1;

    search_reset_nodes();

    Move best = search_best_move_timed(
        &board, limits, uci_on_search_info, uci_poll_stdin
    );

    printf("bestmove ");
    print_uci_move(best);
    printf("\n");

    fflush(stdout);
}


void uci_loop(void)
{
    /*
     * Unbuffered stdin: uci_poll_stdin() uses select() to check for
     * pending input during search. With stdio's default block buffering
     * on a pipe, a "stop" sent right after "go" could get swallowed into
     * the buffer ahead of us, and select() would never see it as ready.
     */
    setvbuf(stdin, NULL, _IONBF, 0);

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

            /*
             * A "quit" sent while that search was running was consumed
             * by uci_poll_stdin(), not by the fgets() above -- it will
             * never come back around this loop, so act on it now.
             */
            if (quit_requested)
                break;

        } else if (strncmp(line, "quit", 4) == 0) {

            break;

        } else if (strncmp(line, "stop", 4) == 0) {

            /*
             * Only reached when no search is running -- a "stop" sent
             * mid-search is caught by uci_poll_stdin() instead. Nothing
             * to do here.
             */

        }
    }
}
