std::uint64_t perft(chess::Board& board, int depth) {
    if (depth == 0) return 1;

    chess::MoveList moves;
    board.generateLegalMoves(moves);

    if (depth == 1)
        return moves.size();

    std::uint64_t nodes = 0;
    for (auto m : moves) {
        auto undo = board.makeMove(m);
        nodes += perft(board, depth - 1);
        board.unmakeMove(m, undo);
    }
    return nodes;
}

std::uint64_t perftDebug(chess::Board& board, int depth, int maxDepth) {
    using namespace chess;

    // Snapshot current position
    std::string fenBefore = board.toFEN();

    MoveList moves;
    board.generateLegalMoves(moves);

    if (depth == 0) {
        // Should never be reached from the top; kept for completeness
        return 1;
    }

    if (depth == 1) {
        std::uint64_t nodes = 0;

        for (Move m : moves) {
            auto undo = board.makeMove(m);

            board.unmakeMove(m, undo);

            std::string fenAfter = board.toFEN();
            if (fenAfter != fenBefore) {
                std::cerr << "\nSTATE CORRUPTION at depth 1\n";
                std::cerr << "Move: " << board.moveToUCI(m) << "\n";
                std::cerr << "Before: " << fenBefore << "\n";
                std::cerr << "After:  " << fenAfter << "\n";
                std::exit(1);
            }

            ++nodes;
        }

        return nodes;
    }

    std::uint64_t nodes = 0;

    for (Move m : moves) {
        auto undo = board.makeMove(m);

        nodes += perftDebug(board, depth - 1, maxDepth);

        board.unmakeMove(m, undo);

        std::string fenAfter = board.toFEN();
        if (fenAfter != fenBefore) {
            std::cerr << "\nSTATE CORRUPTION detected\n";
            std::cerr << "At recursive depth: " << (maxDepth - depth + 1) << "\n";
            std::cerr << "After unmaking move: " << board.moveToUCI(m) << "\n";
            std::cerr << "Expected FEN: " << fenBefore << "\n";
            std::cerr << "Got FEN:      " << fenAfter << "\n";
            std::exit(1);
        }
    }

    return nodes;
}

void perftDivide(chess::Board& board, int depth) {
    chess::MoveList moves;
    board.generateLegalMoves(moves);

    std::uint64_t total = 0;

    for (auto m : moves) {
        auto undo = board.makeMove(m);
        std::uint64_t nodes = perft(board, depth - 1);
        board.unmakeMove(m, undo);

        std::cout << board.moveToUCI(m) << ": " << nodes << "\n";
        total += nodes;
    }

    std::cout << "Total: " << total << "\n";
}

//-----------------------------------------------------------------------------
// UCI loop - Complete implementation for GUI compatibility (CuteChess, Arena, etc.)
//-----------------------------------------------------------------------------
