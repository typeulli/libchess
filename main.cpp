#include "src/libchess.hpp"
#include <iostream>

int main() {
    using namespace chess;

    Board board;
    std::cout << "Initial position:\n" << board << "\n";

    std::string move_str;
    while (true) {
        std::cout << "Enter a move in UCI format (e.g. e2e4), or 'quit' to exit: ";
        std::cin >> move_str;
        if (move_str == "quit") break;

        try {
            board.push_uci(move_str);
            std::cout << "After move " << move_str << ":\n" << board << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Invalid move: " << e.what() << "\n";
        }
    }

    return 0;
}