# ThisBot

UCI-compatible chess engine with simple heuristics, written in C++.

A compact, educational chess engine implementing a UCI (Universal Chess Interface) front-end, move generation, evaluation heuristics, and a basic search (e.g., minimax/alpha-beta with pruning). Designed for experimentation, learning, and integration with UCI-compatible GUIs and tools.

---

## Features

- Implements the UCI protocol for easy integration with GUIs and testing tools.
- Move generation and legal-move checking for standard chess.
- Moderately complex heuristic evaluation.
- Search algorithm with alpha-beta pruning and move ordering.
- Small, readable C++ codebase intended for contributors and learners.

---

## Table of Contents

- Getting started
  - Requirements
  - Build instructions
- Usage
  - Running the engine (UCI)
  - Common UCI commands
  - Example: running a match with cutechess-cli
- Engine options & configuration
- Development notes
  - Suggested extension ideas
- Contributing
- License

---

## Getting started

### Requirements

- C++ compiler with C++17 support (g++, clang++, MSVC).
- A UCI-compatible GUI or tool to interact with the engine (e.g., Arena, Cute Chess GUI, cutechess-cli).

### Build

g++ -std=c++17 -O3 -march=native -DNDEBUG -o thisbot

If you run into build errors, open an issue.

---

## Usage

The engine communicates via UCI on stdin/stdout. Run the engine binary from a terminal or configure your GUI to use the engine executable.

Example: start the engine from a terminal
./thisbot

UCI handshake:

uci
isready
ucinewgame
position startpos moves e2e4 e7e5
go depth 10

UCI commands the engine will understand:
- uci — initialize UCI mode and report engine info/options.
- isready — wait for "readyok" before sending further commands.
- ucinewgame — signal a new game.
- position [fen | startpos] [moves ...] — set the board position.
- go [depth X | movetime ms | wtime ms btime ms | infinite | nodes X] — start search.
- stop — stop searching and return best move.
- quit — exit the engine.

---

## Engine options & configuration

The engine exposes UCI options such as:
- Hash — transposition table size (MB)

setoption name <OptionName> value <Value>

Example:
setoption name Hash value 64

---

## Contributing

Contributions, bug reports, and pull requests are welcome.

- Fork the repository.
- Create a feature branch for your changes.
- Keep changes small and focused. Add tests where appropriate.
- Open a pull request describing the change, rationale, and any performance implications.

Please follow typical GitHub contribution etiquette and include tests or reproducible steps for bugs.

---

## License

MIT
