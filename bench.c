#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "board.h"
#include "search.h"
static double elapsed_seconds(
    struct timespec start,
    struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}


static void print_move(Move m) {
    printf("%c%d%c%d",
           'a' + m.from_col,
           m.from_row + 1,
           'a' + m.to_col,
           m.to_row + 1);

    if (m.type == PROMO) {
        switch (m.promo_piece) {
            case QUEEN:
                putchar('q');
                break;

            case ROOK:
                putchar('r');
                break;

            case BISHOP:
                putchar('b');
                break;

            case KNIGHT:
                putchar('n');
                break;

            default:
                putchar('?');
                break;
        }
    }
}


static void benchmark_depth(
    Board* b,
    int depth) {
    /*
     * Make sure every depth starts from the exact
     * same position.
     */
    Board original = *b;

    search_reset_nodes();

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    Move best_move = search_best_move(b, depth);

    clock_gettime(CLOCK_MONOTONIC, &end);

    /*
     * Search should never modify the position permanently.
     */
    if (memcmp(b, &original, sizeof(Board)) != 0) {
        fprintf(stderr,
                "ERROR: search corrupted the board at depth %d\n",
                depth);

        exit(EXIT_FAILURE);
    }

    uint64_t nodes = search_get_nodes();

    double seconds =
        elapsed_seconds(start, end);

    double nps = 0.0;

    if (seconds > 0.0)
        nps = (double)nodes / seconds;

    printf(
        "depth %d  "
        "nodes %12" PRIu64 "  "
        "time %.3f s  "
        "nps %12.0f  "
        "best ",
        depth,
        nodes,
        seconds,
        nps
    );

    print_move(best_move);

    putchar('\n');
}


int main(int argc, char** argv) {
    int max_depth = 4;

    if (argc >= 2) {
        max_depth = atoi(argv[1]);

        if (max_depth < 1) {
            fprintf(stderr,
                    "Depth must be >= 1\n");

            return EXIT_FAILURE;
        }
    }

    Board b;
    board_init_startpos(&b);

    printf("Plain Negamax Benchmark\n");
    printf("=======================\n");
    printf("Position: starting position\n");
    printf("Max depth: %d\n\n", max_depth);

    for (int depth = 1; depth <= max_depth; depth++) {
        benchmark_depth(&b, depth);
    }

    return EXIT_SUCCESS;
}
