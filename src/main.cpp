// clang-format off

#include "board.hpp"

Board board;

#include "search.hpp"
#include "uci.hpp"

int main()
{
    std::cout << "Starzix by zzzzz" << std::endl;
    attacks::init();
    search::init();
    board = Board(START_FEN);
    uci::uciLoop();
    return 0;
}


