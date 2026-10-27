# darwins-chess

A chess engine in C, with a genetic algorithm planned on top to tune its eval.

The eval is material plus piece-square tables (one table per piece), which is a hand-written guess at who's winning. The idea is to treat those ~389 numbers as a genome and let self-play evolve them: genomes play each other, the winners breed, and a tiny fraction of the genes mutate. Each generation's best genome then plays the frozen generation-0 eval. GA's is metaheuristic, so it's not like its meant to find the most optimised solution anyways.

This started as a shower thought, so I'm not chasing Elo. I wanted to see how a GA actually behaves as an optimiser: how noisy fitness is at ~10 games per genome, and whether the curve goes anywhere. For 389 weights it's the wrong tool. I've came across many modern approaches like Texel tuning (gradient descent on eval error against game outcomes) and learnt it is far cheaper and almost certainly better. I'm doing it anyway because I want to watch the evolution happen.

It's not RL either. The GA never learns which move or which weight won a game. It only sees one score per genome ("this vector got 6.5/10"), and it treats the engine as a black box.

The GA is not written yet. Everything below describes the engine it will run on. Currently caught up with exams and procastinating this project for months lol

## Status

- Engine works. Plays legal games over UCI, tested in CuteChess.
- Perft: 20/20 positions pass.
- Search: ~340k nps, depth 7 from startpos in ~3.4 s (one run, Ryzen 7 PRO 8840U, gcc 16.2.1 `-O3`).
- GA: not implemented, main thing tho
- Elo: not measured yet, tried, but takes too much time, ill say 1600+ atleast.

## Features

- 8x8 mailbox board (`int board[8][8]`), FEN parsing, make/unmake with undo state
- Legal movegen: castling, en passant, all four promotions. Pseudo-legal generation, then make/unmake legality filter
- Negamax + alpha-beta
- Iterative deepening, keeps the last completed depth as fallback
- Quiescence search
- Zobrist hashing (incremental) + transposition table (2^20 entries, always-replace)
- Move ordering: TT move first, then MVV-LVA
- Time management: `go wtime/btime` becomes a budget, no new depth once half of it is gone. `go depth N` also works
- `stop` / `quit` polled mid-search
- UCI: `info depth/score/nps/time/pv` every iteration
- Eval: material + PST, one table per piece
- Perft suite (20 positions), bench

## Perft

```
startpos    d5    4,865,609   0.49 s
kiwipete    d4    4,085,603   0.30 s
position 4  d5   15,833,292   1.12 s
```

## Build

```sh
make          # ./chess, speaks UCI
make perft
make bench    # ./bench [max_depth]
```

## Reference

[Chess Programming Wiki](https://chessprogramming.org/)

[Alogrithm Afternoon / Genetic Algorithm](https://algorithmafternoon.com/)