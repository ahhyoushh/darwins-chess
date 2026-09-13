CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3

CORE_SRC = board.c movegen.c eval.c search.c
ENGINE_SRC = $(CORE_SRC) uci.c main.c
PERFT_SRC = $(CORE_SRC) perft.c test_perft.c
SEARCH_TEST_SRC = $(CORE_SRC) test_search.c
BENCH_SRC = $(CORE_SRC) bench.c

.PHONY: all clean

all: chess

chess: $(ENGINE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

perft: $(PERFT_SRC)
	$(CC) $(CFLAGS) $^ -o $@

search: $(SEARCH_TEST_SRC)
	$(CC) $(CFLAGS) $^ -o $@

bench: $(BENCH_SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f chess perft search bench
