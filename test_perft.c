#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "tt.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

typedef enum {
    TEST_CORE,
    TEST_EDGE
} TestGroup;

typedef struct {
    const char *name;
    const char *fen;
    int depth;
    uint64_t expected;
    TestGroup group;
} PerftCase;


/*
 * ============================================================
 * STANDARD PERFT SUITE
 * ============================================================
 *
 * These are standard chess-engine test positions.
 *
 * Core:
 *   startpos
 *   Kiwipete
 *   positions 3-6
 *
 * Edge:
 *   en passant legality
 *   castling through check
 *   castling rights
 *   promotion/check interactions
 *   discovered checks
 *   stalemate/checkmate
 *
 * Add future positions here rather than modifying main().
 */

static const PerftCase TESTS[] = {

    /*
     * --------------------------------------------------------
     * CORE
     * --------------------------------------------------------
     */

    {
        "startpos",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        5,
        4865609ULL,
        TEST_CORE
    },

    {
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/"
        "2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        4,
        4085603ULL,
        TEST_CORE
    },

    {
        "position3",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 "
        "w - - 0 1",
        5,
        674624ULL,
        TEST_CORE
    },

    {
        "position4",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/"
        "q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        5,
        15833292ULL,
        TEST_CORE
    },

    {
        "position5",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/"
        "PPP1NnPP/RNBQK2R w KQ - 1 8",
        4,
        2103487ULL,
        TEST_CORE
    },

    {
        "position6",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/"
        "2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        4,
        3894594ULL,
        TEST_CORE
    },


    /*
     * --------------------------------------------------------
     * EN PASSANT / CASTLING / CHECK / PROMOTION EDGE CASES
     * --------------------------------------------------------
     */

    {
        "illegal_ep_1",
        "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
        6,
        1134888ULL,
        TEST_EDGE
    },

    {
        "illegal_ep_2",
        "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
        6,
        1015133ULL,
        TEST_EDGE
    },

    {
        "ep_capture_gives_check",
        "8/8/1k6/2b5/2pP4/8/5K2/8 "
        "b - d3 0 1",
        6,
        1440467ULL,
        TEST_EDGE
    },

    {
        "short_castle_gives_check",
        "5k2/8/8/8/8/8/8/4K2R "
        "w K - 0 1",
        6,
        661072ULL,
        TEST_EDGE
    },

    {
        "long_castle_gives_check",
        "3k4/8/8/8/8/8/8/R3K3 "
        "w Q - 0 1",
        6,
        803711ULL,
        TEST_EDGE
    },

    {
        "castle_rights",
        "r3k2r/1b4bq/8/8/8/8/7B/R3K2R "
        "w KQkq - 0 1",
        4,
        1274206ULL,
        TEST_EDGE
    },

    {
        "castling_prevented",
        "r3k2r/8/3Q4/8/8/5q2/8/"
        "R3K2R b KQkq - 0 1",
        4,
        1720476ULL,
        TEST_EDGE
    },

    {
        "promote_out_of_check",
        "2K2r2/4P3/8/8/8/8/8/3k4 "
        "w - - 0 1",
        6,
        3821001ULL,
        TEST_EDGE
    },

    {
        "discovered_check",
        "8/8/1P2K3/8/2n5/1q6/8/5k2 "
        "b - - 0 1",
        5,
        1004658ULL,
        TEST_EDGE
    },

    {
        "promote_to_give_check",
        "4k3/1P6/8/8/8/8/K7/8 "
        "w - - 0 1",
        6,
        217342ULL,
        TEST_EDGE
    },

    {
        "underpromote_to_give_check",
        "8/P1k5/K7/8/8/8/8/8 "
        "w - - 0 1",
        6,
        92683ULL,
        TEST_EDGE
    },

    {
        "self_stalemate",
        "K1k5/8/P7/8/8/8/8/8 "
        "w - - 0 1",
        6,
        2217ULL,
        TEST_EDGE
    },

    {
        "stalemate_checkmate_1",
        "8/k1P5/8/1K6/8/8/8/8 "
        "w - - 0 1",
        7,
        567584ULL,
        TEST_EDGE
    },

    {
        "stalemate_checkmate_2",
        "8/8/2k5/5q2/5n2/8/5K2/8 "
        "b - - 0 1",
        4,
        23527ULL,
        TEST_EDGE
    }
};


/*
 * ============================================================
 * HELPERS
 * ============================================================
 */

static bool move_equal(Move a, Move b) {
    return
        a.from_row == b.from_row &&
        a.from_col == b.from_col &&
        a.to_row == b.to_row &&
        a.to_col == b.to_col &&
        a.type == b.type &&
        a.promo_piece == b.promo_piece;
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


static bool board_equal(const Board *a, const Board *b) {
    return memcmp(a, b, sizeof(Board)) == 0;
}


/*
 * Check that the legal move list itself looks sane.
 *
 * This is separate from perft because it catches structural
 * problems immediately instead of only seeing a wrong node count.
 */
static bool validate_legal_moves(Board *b, const char *name) {
    MoveList moves;
    generate_legal_moves(b, &moves);

    if (moves.count < 0 || moves.count > MAX_MOVES) {
        fprintf(stderr,
                "[FAIL] %s: invalid move count: %d\n",
                name,
                moves.count);

        return false;
    }

    for (int i = 0; i < moves.count; i++) {
        Move a = moves.moves[i];

        /*
         * Do not allow moves which capture the enemy king.
         */
        int moving_piece =
            b->squares[a.from_row][a.from_col];

        int target =
            b->squares[a.to_row][a.to_col];

        int color =
            (moving_piece > 0) ? WHITE : BLACK;

        if (target == -color * KING) {
            fprintf(stderr,
                    "[FAIL] %s: move captures enemy king: ",
                    name);

            print_move(a);
            putchar('\n');

            return false;
        }

        /*
         * Check for duplicate moves.
         */
        for (int j = 0; j < i; j++) {
            if (move_equal(a, moves.moves[j])) {
                fprintf(stderr,
                        "[FAIL] %s: duplicate move: ",
                        name);

                print_move(a);
                putchar('\n');

                return false;
            }
        }
    }

    return true;
}


/*
 * Every legal root move should round-trip perfectly through:
 *
 *     make_move()
 *     unmake_move()
 *
 * This is especially useful for:
 *
 *     promotion
 *     en passant
 *     castling
 *     captures
 *     king moves
 */
static bool test_make_unmake(Board *b, const char *name) {
    MoveList moves;
    generate_legal_moves(b, &moves);

    Board original = *b;

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        Undo u;

        board_make_move(b, m, &u);
        board_unmake_move(b, m, &u);

        if (!board_equal(b, &original)) {
            fprintf(stderr,
                    "[FAIL] %s: board corrupted by make/unmake: ",
                    name);

            print_move(m);
            putchar('\n');

            return false;
        }
    }

    return true;
}


/*
 * Divide is the most useful debugging tool when a perft
 * number is wrong.
 */
static void divide(Board *b, int depth) {
    MoveList moves;
    generate_legal_moves(b, &moves);

    uint64_t total = 0;

    printf("\nDivide depth %d:\n", depth);

    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        Undo u;

        board_make_move(b, m, &u);

        uint64_t nodes = perft(b, depth - 1);

        board_unmake_move(b, m, &u);

        print_move(m);
        printf(": %" PRIu64 "\n", nodes);

        total += nodes;
    }

    printf("Total: %" PRIu64 "\n\n", total);
}


/*
 * Run one perft case.
 */
static bool run_case(const PerftCase *tc, bool do_divide) {
    Board b;

    if (!board_load_fen(&b, tc->fen)) {
        fprintf(stderr,
                "[FAIL] %s: FEN rejected\n",
                tc->name);

        return false;
    }

    /*
     * Structural checks before expensive perft.
     */
    if (!validate_legal_moves(&b, tc->name))
        return false;

    if (!test_make_unmake(&b, tc->name))
        return false;

    /*
     * Save the entire position.
     */
    Board before = b;

    clock_t start = clock();

    uint64_t actual = perft(&b, tc->depth);

    clock_t end = clock();

    /*
     * Perft must leave the board exactly as it was.
     */
    if (!board_equal(&b, &before)) {
        fprintf(stderr,
                "[FAIL] %s: board changed after perft()\n",
                tc->name);

        return false;
    }

    double seconds =
        (double)(end - start) / (double)CLOCKS_PER_SEC;

    if (actual != tc->expected) {

        printf("[FAIL] %-28s "
               "depth=%d "
               "expected=%" PRIu64 " "
               "actual=%" PRIu64 " "
               "time=%.3fs\n",
               tc->name,
               tc->depth,
               tc->expected,
               actual,
               seconds);

        if (do_divide)
            divide(&b, tc->depth);

        return false;
    }

    printf("[PASS] %-28s "
           "depth=%d "
           "nodes=%" PRIu64 " "
           "time=%.3fs\n",
           tc->name,
           tc->depth,
           actual,
           seconds);

    return true;
}


/*
 * Find a test by name.
 */
static const PerftCase *find_case(const char *name) {
    for (size_t i = 0; i < ARRAY_LEN(TESTS); i++) {
        if (strcmp(TESTS[i].name, name) == 0)
            return &TESTS[i];
    }

    return NULL;
}


static void print_usage(const char *program) {
    printf(
        "Usage:\n"
        "  %s                  Run full suite\n"
        "  %s --quick          Run core suite\n"
        "  %s --edge           Run edge-case suite\n"
        "  %s --case NAME      Run one test\n"
        "  %s --divide NAME    Run one test and print divide\n"
        "  %s --help           Show this help\n",
        program,
        program,
        program,
        program,
        program,
        program
    );
}


/*
 * ============================================================
 * SMALL DIRECT FEATURE TESTS
 * ============================================================
 *
 * These don't rely on perft counts. They directly verify that
 * specific move types appear in the move list.
 */

static bool has_move(
    const MoveList *list,
    int fr, int fc,
    int tr, int tc,
    MoveType type) {
    for (int i = 0; i < list->count; i++) {
        Move m = list->moves[i];

        if (m.from_row == fr &&
            m.from_col == fc &&
            m.to_row == tr &&
            m.to_col == tc &&
            m.type == type) {

            return true;
        }
    }

    return false;
}


static bool test_special_move_generation(void) {
    Board b;
    MoveList moves;

    /*
     * --------------------------------------------------------
     * EN PASSANT
     *
     * White pawn e5 -> d6
     * --------------------------------------------------------
     */
    if (!board_load_fen(
            &b,
            "4k3/8/8/3pP3/8/8/8/4K3 "
            "w - d6 0 1")) {

        fprintf(stderr,
                "[FAIL] special: could not load EP FEN\n");

        return false;
    }

    generate_legal_moves(&b, &moves);

    if (!has_move(
            &moves,
            4, 4,       /* e5 */
            5, 3,       /* d6 */
            EN_PASSANT)) {

        fprintf(stderr,
                "[FAIL] special: e5xd6 en passant missing\n");

        return false;
    }


    /*
     * --------------------------------------------------------
     * CASTLING
     *
     * White should have both O-O and O-O-O.
     * --------------------------------------------------------
     */
    if (!board_load_fen(
            &b,
            "4k2r/8/8/8/8/8/8/"
            "R3K2R w KQk - 0 1")) {

        fprintf(stderr,
                "[FAIL] special: could not load castle FEN\n");

        return false;
    }

    generate_legal_moves(&b, &moves);

    if (!has_move(
            &moves,
            0, 4,       /* e1 */
            0, 6,       /* g1 */
            CASTLE_KS)) {

        fprintf(stderr,
                "[FAIL] special: white O-O missing\n");

        return false;
    }

    if (!has_move(
            &moves,
            0, 4,       /* e1 */
            0, 2,       /* c1 */
            CASTLE_QS)) {

        fprintf(stderr,
                "[FAIL] special: white O-O-O missing\n");

        return false;
    }


    /*
     * --------------------------------------------------------
     * UNDERPROMOTION
     *
     * White pawn a7 should generate:
     *
     *     a8=Q
     *     a8=R
     *     a8=B
     *     a8=N
     * --------------------------------------------------------
     */
    if (!board_load_fen(
            &b,
            "4k3/P7/8/8/8/8/8/4K3 "
            "w - - 0 1")) {

        fprintf(stderr,
                "[FAIL] special: could not load promotion FEN\n");

        return false;
    }

    generate_legal_moves(&b, &moves);

    int promotions = 0;

    for (int i = 0; i < moves.count; i++) {
        if (moves.moves[i].type == PROMO)
            promotions++;
    }

    if (promotions != 4) {
        fprintf(stderr,
                "[FAIL] special: expected 4 promotions, got %d\n",
                promotions);

        return false;
    }

    printf("[PASS] special move generation\n");

    return true;
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(int argc, char **argv) {
    tt_init();
    bool run_core = false;
    bool run_edge = false;
    bool run_all = false;

    const PerftCase *single_case = NULL;
    bool single_divide = false;

    if (argc == 1) {
        run_all = true;
    } else {

        for (int i = 1; i < argc; i++) {

            if (strcmp(argv[i], "--help") == 0) {
                print_usage(argv[0]);
                return 0;
            }

            if (strcmp(argv[i], "--quick") == 0) {
                run_core = true;
                continue;
            }

            if (strcmp(argv[i], "--edge") == 0) {
                run_edge = true;
                continue;
            }

            if (strcmp(argv[i], "--case") == 0) {

                if (i + 1 >= argc) {
                    fprintf(stderr,
                            "--case requires a test name\n");

                    return 1;
                }

                single_case = find_case(argv[++i]);

                if (single_case == NULL) {
                    fprintf(stderr,
                            "Unknown test: %s\n",
                            argv[i]);

                    return 1;
                }

                continue;
            }

            if (strcmp(argv[i], "--divide") == 0) {

                if (i + 1 >= argc) {
                    fprintf(stderr,
                            "--divide requires a test name\n");

                    return 1;
                }

                single_case = find_case(argv[++i]);

                if (single_case == NULL) {
                    fprintf(stderr,
                            "Unknown test: %s\n",
                            argv[i]);

                    return 1;
                }

                single_divide = true;
                continue;
            }

            fprintf(stderr,
                    "Unknown option: %s\n\n",
                    argv[i]);

            print_usage(argv[0]);
            return 1;
        }
    }


    /*
     * Direct feature tests first.
     */
    if (!test_special_move_generation())
        return 1;


    /*
     * One specific case.
     */
    if (single_case != NULL) {
        bool ok = run_case(single_case, true);

        if (single_divide)
            return ok ? 0 : 1;

        return ok ? 0 : 1;
    }


    /*
     * Normal suites.
     */
    size_t passed = 0;
    size_t failed = 0;

    for (size_t i = 0; i < ARRAY_LEN(TESTS); i++) {

        if (!run_all) {
            if (TESTS[i].group == TEST_CORE && !run_core)
                continue;

            if (TESTS[i].group == TEST_EDGE && !run_edge)
                continue;
        }

        /*
         * On normal runs, automatically print divide when
         * something fails.
         */
        bool ok = run_case(&TESTS[i], true);

        if (ok)
            passed++;
        else
            failed++;
    }


    /*
     * Summary.
     */
    printf("\n========================================\n");
    printf("Perft test summary\n");
    printf("Passed: %zu\n", passed);
    printf("Failed: %zu\n", failed);
    printf("========================================\n");

    if (failed != 0)
        return 1;

    return 0;
}
