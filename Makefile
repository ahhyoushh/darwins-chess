CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3

SRC = board.c movegen.c eval.c search.c perft.c

all: chess

chess: $(SRC) main.c
	$(CC) $(CFLAGS) $^ -o $@

perft: $(SRC) test_perft.c
	$(CC) $(CFLAGS) $^ -o $@

search: $(SRC) test_search.c
	$(CC) $(CFLAGS) $^ -o $@

bench: $(SRC) bench.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f chess perft search bench
