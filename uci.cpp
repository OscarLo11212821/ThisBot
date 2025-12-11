int main() {
    using namespace chess;
    
    // Disable buffering for immediate output to GUI
    std::cout.setf(std::ios::unitbuf);
    std::cin.tie(nullptr);
    
    Board board;
    ThisBot bot;
    bool debugMode = false;
    
    // Engine info
    const std::string ENGINE_NAME = "This Bot v0.1";
    const std::string ENGINE_AUTHOR = "oscar128372";

    auto tokenize = [](const std::string& line) {
        std::vector<std::string> out;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) out.push_back(tok);
        return out;
    };

    auto setPosition = [&](const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) return;
        size_t idx = 1;
        if (tokens[idx] == "startpos") {
            board.reset();
            ++idx;
        } else if (tokens[idx] == "fen") {
            std::string fen;
            ++idx;
            while (idx < tokens.size() && tokens[idx] != "moves") {
                if (!fen.empty()) fen += ' ';
                fen += tokens[idx++];
            }
            if (!fen.empty()) board.setFEN(fen);
        }

        auto applyMoves = [&](size_t startIdx) {
            for (size_t i = startIdx; i < tokens.size(); ++i) {
                auto mv = board.parseUCI(tokens[i]);
                if (mv) board.makeMove(*mv);
            }
        };

        for (; idx < tokens.size(); ++idx) {
            if (tokens[idx] == "moves") { applyMoves(idx + 1); break; }
        }
    };

    auto handleGo = [&](const std::vector<std::string>& t) {
        int movetime = -1, wtime = -1, btime = -1, winc = 0, binc = 0;
        int depth = 128, movestogo = -1;
        std::uint64_t nodes = 0;
        bool infinite = false;
        bool ponder = false;
        bool perftMode = false;
        int perftDepth = 0;

        for (size_t i = 1; i < t.size(); ++i) {
            if (t[i] == "infinite") infinite = true;
            else if (t[i] == "ponder") ponder = true;
            else if (t[i] == "perft" && i + 1 < t.size()) { perftMode = true; perftDepth = std::stoi(t[++i]); }
            else if (i + 1 < t.size()) {
                if (t[i] == "movetime") movetime = std::stoi(t[++i]);
                else if (t[i] == "wtime") wtime = std::stoi(t[++i]);
                else if (t[i] == "btime") btime = std::stoi(t[++i]);
                else if (t[i] == "winc") winc = std::stoi(t[++i]);
                else if (t[i] == "binc") binc = std::stoi(t[++i]);
                else if (t[i] == "depth") depth = std::stoi(t[++i]);
                else if (t[i] == "nodes") nodes = std::stoull(t[++i]);
                else if (t[i] == "movestogo") movestogo = std::stoi(t[++i]);
                else if (t[i] == "mate") { /* mate in N - could limit search */ }
            }
        }

        // Handle perft command within go
        if (perftMode) {
            auto startTime = std::chrono::steady_clock::now();
            std::uint64_t total = perft(board, perftDepth);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            auto nps = elapsed > 0 ? (total * 1000ULL) / elapsed : 0;
            std::cout << "info nodes " << total << " time " << elapsed << " nps " << nps << std::endl;
            std::cout << "bestmove 0000" << std::endl;
            return;
        }

        int side = board.sideToMove_ == WHITE ? 0 : 1;
        int timeLeft = side == 0 ? wtime : btime;
        int inc = side == 0 ? winc : binc;

        int softMs = 0, hardMs = 0;
        
        bool fixedSearch = (depth != 128 || nodes != 0);

        if (infinite || ponder || fixedSearch) {
            // Search until stopped, or until depth/node limit is reached
            softMs = 0;
            hardMs = 0;
        } else if (movetime > 0) {
            // Fixed time per move
            softMs = hardMs = movetime;
        } else if (wtime > 0 || btime > 0) {
            // Time management for games with clocks
            int side = board.sideToMove_ == WHITE ? 0 : 1;
            int timeLeft = side == 0 ? wtime : btime;
            int inc = side == 0 ? winc : binc;

            if (movestogo > 0) {
                softMs = timeLeft / (movestogo + 2);
                hardMs = timeLeft / std::max(1, movestogo / 2);
            } else {
                softMs = timeLeft / 40;
                if (inc > 0) softMs += inc * 3 / 4;
                hardMs = std::min(timeLeft / 4, softMs * 5);
            }
            // Safety margin
            softMs = std::max(1, softMs - 10);
            hardMs = std::max(1, hardMs - 50);
             if (board.halfmove_ > 80) {
                int urgency = std::max(1, 100 - board.halfmove_);
                softMs = std::min(timeLeft / 2, softMs + std::max(5, softMs * urgency / 40));
                hardMs = std::min(timeLeft / 2, std::max(hardMs, softMs * 2));
            }
        } else {
            // Fallback: if no time controls and no depth/node limits, use a default
            softMs = 1000;
            hardMs = 5000;
        }

        Move best = bot.think(board, softMs, hardMs, depth, nodes);
        std::cout << "bestmove " << board.moveToUCI(best) << std::endl;
    };

    auto handleSetOption = [&](const std::vector<std::string>& t) {
        // Parse: setoption name <name> [value <value>]
        std::string name, value;
        bool readingName = false, readingValue = false;
        
        for (size_t i = 1; i < t.size(); ++i) {
            if (t[i] == "name") { readingName = true; readingValue = false; continue; }
            if (t[i] == "value") { readingName = false; readingValue = true; continue; }
            if (readingName) {
                if (!name.empty()) name += " ";
                name += t[i];
            }
            if (readingValue) {
                if (!value.empty()) value += " ";
                value += t[i];
            }
        }
        
        // Convert name to lowercase for case-insensitive matching
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName == "hash") {
            int mbSize = std::stoi(value);
            mbSize = std::max(1, std::min(mbSize, 16384));  // Clamp to 1-16GB
            bot.setHashSize(static_cast<size_t>(mbSize));
            if (debugMode) {
                std::cout << "info string Hash set to " << mbSize << " MB" << std::endl;
            }
        } else if (lowerName == "clear hash") {
            bot.setHashSize(bot.getHashSize());  // Clear by resizing to same size
            if (debugMode) {
                std::cout << "info string Hash cleared" << std::endl;
            }
        }
        // Other options silently ignored for compatibility
    };

    auto printUciOptions = []() {
        std::cout << "option name Hash type spin default 128 min 1 max 16384" << std::endl;
        std::cout << "option name Clear Hash type button" << std::endl;
        // Add more options here as needed
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start, end - start + 1);
        
        if (line.empty()) continue;
        
        auto tokens = tokenize(line);
        if (tokens.empty()) continue;
        
        const std::string& cmd = tokens[0];

        if (cmd == "uci") {
            std::cout << "id name " << ENGINE_NAME << std::endl;
            std::cout << "id author " << ENGINE_AUTHOR << std::endl;
            printUciOptions();
            std::cout << "uciok" << std::endl;
        }
        else if (cmd == "debug") {
            if (tokens.size() > 1) {
                debugMode = (tokens[1] == "on");
            }
        }
        else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (cmd == "setoption") {
            handleSetOption(tokens);
        }
        else if (cmd == "register") {
            // Engine registration, we don't require it
            std::cout << "registration ok" << std::endl;
        }
        else if (cmd == "ucinewgame") {
            board.reset();
            // Clear hash table for new game
            bot.setHashSize(bot.getHashSize());
        }
        else if (cmd == "position") {
            setPosition(tokens);
        }
        else if (cmd == "go") {
            handleGo(tokens);
        }
        else if (cmd == "stop") {
            bot.stop();
        }
        else if (cmd == "ponderhit") {
            // We don't support pondering yet
        }
        else if (cmd == "quit") {
            break;
        }
        // Non-standard but useful commands
        else if (cmd == "d" || cmd == "display") {
            board.print();
        }
        else if (cmd == "perft") {
            if (tokens.size() > 1) {
                int d = std::stoi(tokens[1]);
                auto startTime = std::chrono::steady_clock::now();
                std::uint64_t total = perft(board, d);
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime).count();
                auto nps = elapsed > 0 ? (total * 1000ULL) / elapsed : 0;
                std::cout << "Nodes: " << total << std::endl;
                std::cout << "Time: " << elapsed << " ms" << std::endl;
                std::cout << "NPS: " << nps << std::endl;
            }
        }
        else if (cmd == "divide") {
            if (tokens.size() > 1) {
                int d = std::stoi(tokens[1]);
                perftDivide(board, d);
            }
        }
        else if (cmd == "fen") {
            std::cout << board.toFEN() << std::endl;
        }
        else if (cmd == "moves") {
            MoveList moves;
            board.generateLegalMoves(moves);
            std::cout << "Legal moves (" << moves.size() << "):" << std::endl;
            for (int i = 0; i < moves.size(); ++i) {
                std::cout << board.moveToUCI(moves[i]);
                if (i < moves.size() - 1) std::cout << " ";
            }
            std::cout << std::endl;
        }
        else if (cmd == "spsa") {
            SpsaConfig cfg;
            cfg.threads = static_cast<int>(std::thread::hardware_concurrency());

            for (size_t i = 1; i + 1 < tokens.size(); i += 2) {
                const std::string& key = tokens[i];
                const std::string& val = tokens[i + 1];
                if (key == "iters" || key == "iterations") cfg.iterations = std::stoi(val);
                else if (key == "games") cfg.gamesPerIteration = std::stoi(val);
                else if (key == "movetime") cfg.moveTimeMs = std::stoi(val);
                else if (key == "depth") cfg.searchDepth = std::stoi(val);
                else if (key == "plies") cfg.maxPlies = std::stoi(val);
                else if (key == "threads") cfg.threads = std::stoi(val);
                else if (key == "a") cfg.a = std::stod(val);
                else if (key == "c") cfg.c = std::stod(val);
                else if (key == "alpha") cfg.alpha = std::stod(val);
                else if (key == "gamma") cfg.gamma = std::stod(val);
                else if (key == "A" || key == "stability") cfg.A = std::stod(val);
                else if (key == "seed") cfg.seed = static_cast<unsigned>(std::stoul(val));
            }

            if (cfg.threads <= 0) cfg.threads = 1;
            std::cout << "info string spsa tuning start" << std::endl;
            SpsaTuner tuner(cfg, bot.evalParams());
            EvalParams tuned = tuner.run();
            bot.setEvalParams(std::make_shared<EvalParams>(tuned));

            auto printArray = [](const std::string& name, const auto& arr) {
                std::cout << "info string " << name << " ";
                for (size_t i = 0; i < arr.size(); ++i) {
                    std::cout << arr[i];
                    if (i + 1 < arr.size()) std::cout << ",";
                }
                std::cout << std::endl;
            };

            printArray("pieceValues", tuned.pieceValues);
            printArray("mobilityBonus", tuned.mobilityBonus);
            printArray("attackWeight", tuned.attackWeight);
            printArray("pieceAttackValue", tuned.pieceAttackValue);
            printArray("passedPawnBonus", tuned.passedPawnBonus);
            printArray("knightOutpostBonus", tuned.knightOutpostBonus);
            printArray("rookFileBonus", tuned.rookFileBonus);
            printArray("developmentWeights", tuned.developmentWeights);
            printArray("kingShieldValues", tuned.kingShieldValues);
            printArray("rookSeventhBonus", tuned.rookSeventhBonus);
            printArray("badBishopPenalty", tuned.badBishopPenalty);
            
            std::cout << "info string spsa tuning complete" << std::endl;
        }
        else if (cmd == "texel") {
            TexelConfig cfg;
            
            for (size_t i = 1; i + 1 < tokens.size(); i += 2) {
                const std::string& key = tokens[i];
                const std::string& val = tokens[i + 1];
                if (key == "iters" || key == "iterations") cfg.iterations = std::stoi(val);
                else if (key == "games") cfg.numGames = std::stoi(val);
                else if (key == "positions") cfg.positionsPerGame = std::stoi(val);
                else if (key == "movetime") cfg.moveTimeMs = std::stoi(val);
                else if (key == "depth") cfg.searchDepth = std::stoi(val);
                else if (key == "plies") cfg.maxPlies = std::stoi(val);
                else if (key == "lr") cfg.learningRate = std::stod(val);
                else if (key == "K") cfg.K = std::stod(val);
                else if (key == "seed") cfg.seed = static_cast<unsigned>(std::stoul(val));
                else if (key == "optimizeK") cfg.optimizeK = (val == "true" || val == "1");
            }
            
            std::cout << "info string texel tuning PSTs" << std::endl;
            
            auto params = std::make_shared<EvalParams>(bot.evalParams());
            TexelTuner tuner(cfg, params);
            tuner.generatePositions();
            tuner.tune();
            bot.setEvalParams(params);
            
            // Print tuned PST values
            auto printPst = [](const std::string& name, const std::array<double, 64>& arr) {
                std::cout << "info string " << name << " {";
                for (int i = 0; i < 64; ++i) {
                    std::cout << static_cast<int>(arr[i]);
                    if (i < 63) std::cout << ",";
                    if (i % 8 == 7 && i < 63) std::cout << " ";
                }
                std::cout << "}" << std::endl;
            };
            
            const char* pstNames[] = {"pstPawn", "pstKnight", "pstBishop", "pstRook", "pstQueen", "pstKingMG"};
            for (int pt = 0; pt < 6; ++pt) {
                printPst(pstNames[pt], params->pst[pt]);
            }
            printPst("pstKingEG", params->kingEndgame);
            
            std::cout << "info string texel tuning complete" << std::endl;
        }
        else if (cmd == "generate" || cmd == "gen") {
            PositionGenConfig cfg;
            
            for (size_t i = 1; i + 1 < tokens.size(); i += 2) {
                const std::string& key = tokens[i];
                const std::string& val = tokens[i + 1];
                if (key == "fenfile" || key == "fen") cfg.fenPath = val;
                else if (key == "out" || key == "output") cfg.outputPath = val;
                else if (key == "games" || key == "playouts") cfg.gamesPerSeed = std::stoi(val);
                else if (key == "positions" || key == "pergame") cfg.positionsPerGame = std::stoi(val);
                else if (key == "plies") cfg.maxPlies = std::stoi(val);
                else if (key == "stride" || key == "every") cfg.sampleStride = std::stoi(val);
                else if (key == "random") cfg.randomPlies = std::stoi(val);
                else if (key == "movesample") cfg.moveSample = std::stoi(val);
                else if (key == "threads") cfg.threads = std::stoi(val);
                else if (key == "seed") cfg.seed = static_cast<unsigned>(std::stoul(val));
                else if (key == "searchlabels") cfg.useSearchLabels = (val == "1" || val == "true");
                else if (key == "labeldepth") cfg.labelDepth = std::stoi(val);
                else if (key == "labelmovetime") cfg.labelMoveTimeMs = std::stoi(val);
                else if (key == "labelnodes") cfg.labelMaxNodes = static_cast<std::uint64_t>(std::stoull(val));
                else if (key == "evalclip") cfg.evalClip = std::stoi(val);
            }
            
            PositionGenerator generator(cfg, std::make_shared<EvalParams>(bot.evalParams()));
            generator.run();
        }
        else if (cmd == "bench") {
            // Simple benchmark - search starting position to fixed depth
            board.reset();
            auto startTime = std::chrono::steady_clock::now();
            bot.think(board, 0, 0, 10, 0);  // depth 10, no time limit
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            std::cout << "Bench completed in " << elapsed << " ms" << std::endl;
        }
    }
    
    return 0;
}
