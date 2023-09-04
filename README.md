# z5 - UCI C++ chess engine

Currently ~2565 elo (tested against [Barbarossa](https://github.com/nionita/Barbarossa) 0.6).

# How to compile

**Linux**

```make``` to create the 'z5' binary

**Windows**

```g++ -O3 -std=c++17 src/main.cpp -o z5.exe```

# UCI options

Hash - transposition table size in MB from 1 to 512 (default 64)

# Features

NNUE evaluation (768->128x2->1)

Iterative deepening

Negamax with alpha beta pruning and PVS

Quiescence search

Transposition table

Move ordering (hash move > SEE + MVVLVA > promotions > killer moves > history heuristic)

Aspiration window

Check extension

Late move reductions

Null move pruning

Reverse futility pruning

Internal iterative reduction

Time management

# Credits

[Chess Programming Wiki](https://www.chessprogramming.org/)

[Cutechess for testing](https://github.com/cutechess/cutechess)

[Disservin's move gen (chess-library)](https://github.com/Disservin/chess-library)

[MantaRay for C++ neural network inference](https://github.com/TheBlackPlague/MantaRay)

[Engine Programming Discord](https://discord.gg/pcjr9eXK)

