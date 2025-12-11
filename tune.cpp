namespace chess {
struct SpsaConfig {
    int iterations = 5;
    int gamesPerIteration = 4;
    int moveTimeMs = 50;
    int searchDepth = 4;
    int maxPlies = 200;
    int threads = static_cast<int>(std::thread::hardware_concurrency());
    double a = 15.0;
    double c = 5.0;
    double alpha = 0.602;
    double gamma = 0.101;
    double A = 10.0;
    unsigned seed = 42;
};

class SpsaTuner {
public:
    SpsaTuner(const SpsaConfig& cfg, const EvalParams& start = EvalParams::defaults())
        : cfg_(cfg), theta_(start), baseline_(start), rng_(cfg.seed) {
        if (cfg_.threads <= 0) cfg_.threads = 1;
        lower_ = flatten(EvalParams::minBounds());
        upper_ = flatten(EvalParams::maxBounds());
    }

    EvalParams run() {
        auto thetaRefs = flatten(theta_);
        std::uniform_int_distribution<int> bit(0, 1);

        for (int iter = 0; iter < cfg_.iterations; ++iter) {
            double ck = cfg_.c / std::pow(iter + 1.0, cfg_.gamma);
            double ak = cfg_.a / std::pow(cfg_.A + iter + 1.0, cfg_.alpha);

            std::vector<int> delta(thetaRefs.size());
            for (size_t i = 0; i < delta.size(); ++i) delta[i] = bit(rng_) ? 1 : -1;

            EvalParams plus = theta_;
            EvalParams minus = theta_;
            applyDelta(plus, delta, ck);
            applyDelta(minus, delta, -ck);

            // Use the same seeds for plus/minus to measure absolute strength vs. baseline with low variance
            std::vector<unsigned> seeds(cfg_.gamesPerIteration);
            for (int g = 0; g < cfg_.gamesPerIteration; ++g) {
                seeds[g] = cfg_.seed + static_cast<unsigned>(iter * cfg_.gamesPerIteration + g);
            }

            double plusScore = runGamesVsBaseline(plus, seeds);
            double minusScore = runGamesVsBaseline(minus, seeds);

            for (size_t i = 0; i < thetaRefs.size(); ++i) {
                double grad = (plusScore - minusScore) * delta[i] / (2.0 * ck);
                *thetaRefs[i] = clamp(*thetaRefs[i] + ak * grad, lower_[i], upper_[i]);
            }

            {
                std::lock_guard<std::mutex> lock(logMutex_);
                std::cout << "info string spsa iter " << (iter + 1)
                          << "/" << cfg_.iterations
                          << " plus " << plusScore
                          << " minus " << minusScore
                          << " ak " << ak
                          << " ck " << ck
                          << std::endl;
            }
        }
        return theta_;
    }

private:
    SpsaConfig cfg_;
    EvalParams theta_;
    EvalParams baseline_;
    std::mt19937 rng_;
    std::vector<double> lower_;
    std::vector<double> upper_;
    std::mutex logMutex_;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    static std::vector<double*> flatten(EvalParams& p) {
        std::vector<double*> refs;
        auto push = [&](auto& arr) { for (auto& v : arr) refs.push_back(&v); };
        push(p.pieceValues);
        push(p.mobilityBonus);
        push(p.attackWeight);
        push(p.pieceAttackValue);
        push(p.passedPawnBonus);
        push(p.knightOutpostBonus);
        push(p.rookFileBonus);
        push(p.developmentWeights);
        push(p.kingShieldValues);
        push(p.rookSeventhBonus);
        push(p.badBishopPenalty);
        // PSTs excluded - tuned by Texel instead
        return refs;
    }

    static std::vector<double> flatten(const EvalParams& p) {
        std::vector<double> vals;
        auto push = [&](const auto& arr) { for (auto v : arr) vals.push_back(v); };
        push(p.pieceValues);
        push(p.mobilityBonus);
        push(p.attackWeight);
        push(p.pieceAttackValue);
        push(p.passedPawnBonus);
        push(p.knightOutpostBonus);
        push(p.rookFileBonus);
        push(p.developmentWeights);
        push(p.kingShieldValues);
        push(p.rookSeventhBonus);
        push(p.badBishopPenalty);
        // PSTs excluded - tuned by Texel instead
        return vals;
    }

    void applyDelta(EvalParams& p, const std::vector<int>& delta, double scale) const {
        auto refs = flatten(p);
        for (size_t i = 0; i < refs.size() && i < delta.size(); ++i) {
            *refs[i] += scale * delta[i];
        }
    }

    double runGamesVsBaseline(const EvalParams& candidate, const std::vector<unsigned>& seeds) {
        std::vector<std::future<double>> tasks;
        tasks.reserve(cfg_.gamesPerIteration);
        double sum = 0.0;

        auto flushOne = [&]() {
            tasks.front().wait();
            sum += tasks.front().get();
            tasks.erase(tasks.begin());
        };

        for (int g = 0; g < cfg_.gamesPerIteration; ++g) {
            bool candidateWhite = (g % 2 == 0);
            unsigned seed = seeds.empty() ? static_cast<unsigned>(cfg_.seed + g) : seeds[g];

            tasks.push_back(std::async(std::launch::async, [this, candidate, candidateWhite, seed]() {
                return playSingleGame(
                    candidateWhite ? candidate : baseline_,
                    candidateWhite ? baseline_ : candidate,
                    candidateWhite,
                    seed);
            }));

            if (cfg_.threads > 0 && static_cast<int>(tasks.size()) >= cfg_.threads) {
                flushOne();
            }
        }

        for (auto& t : tasks) sum += t.get();
        return cfg_.gamesPerIteration == 0 ? 0.0 : sum / cfg_.gamesPerIteration;
    }

    double playSingleGame(const EvalParams& whiteParams, const EvalParams& blackParams, bool plusIsWhite, unsigned seed) const {
        Board board;
        board.reset();

        ThisBot white(std::make_shared<EvalParams>(whiteParams));
        ThisBot black(std::make_shared<EvalParams>(blackParams));
        // Use a small transposition table during tuning to avoid OOM when many games run in parallel
        white.setHashSize(8);  // MB
        black.setHashSize(8);  // MB

        std::mt19937 rng(seed);

        // Small randomization to diversify starting positions
        for (int i = 0; i < 2; ++i) {
            MoveList moves;
            board.generateLegalMoves(moves);
            if (moves.size() == 0) break;
            std::uniform_int_distribution<int> dist(0, moves.size() - 1);
            Move randMove = moves[dist(rng)];
            board.makeMove(randMove);
            if (board.isCheckmate() || board.isStalemate()) break;
        }

        double result = 0.0;
        for (int ply = 0; ply < cfg_.maxPlies; ++ply) {
            MoveList legals;
            board.generateLegalMoves(legals);
            if (legals.size() == 0) {
                result = board.inCheck() ? (board.sideToMove_ == WHITE ? -1.0 : 1.0) : 0.0;
                break;
            }

            ThisBot& engine = board.sideToMove_ == WHITE ? white : black;
            Move mv = engine.think(board, cfg_.moveTimeMs, cfg_.moveTimeMs, cfg_.searchDepth);
            if (mv.isNull()) { result = 0.0; break; }

            auto undo = board.makeMove(mv);
            (void)undo;

            if (board.isCheckmate()) {
                result = board.sideToMove_ == WHITE ? -1.0 : 1.0;
                break;
            }
            if (board.isStalemate() || board.halfmove_ >= 100) {
                result = 0.0;
                break;
            }
        }

        return plusIsWhite ? result : -result;
    }
};

//============================================================================
// Position generator from opening seeds
//============================================================================
struct PositionEvalSample {
    std::string fen;
    int eval;
};

struct PositionGenConfig {
    std::string fenPath = "fen.txt";
    std::string outputPath = "generated_positions.txt";
    int gamesPerSeed = 12;
    int positionsPerGame = 24;
    int maxPlies = 80;
    int sampleStride = 2;
    int randomPlies = 4;
    int moveSample = 6; // how many candidate moves to score before picking the best
    int threads = static_cast<int>(std::thread::hardware_concurrency());
    unsigned seed = 1337;
    // labeling
    bool useSearchLabels = true;
    int labelDepth = 4;
    int labelMoveTimeMs = 40;
    std::uint64_t labelMaxNodes = 0;
    int evalClip = 3000;
};

class PositionGenerator {
public:
    PositionGenerator(const PositionGenConfig& cfg, std::shared_ptr<EvalParams> params)
        : cfg_(cfg), params_(std::move(params)) {
        if (cfg_.threads <= 0) cfg_.threads = 1;
        if (cfg_.gamesPerSeed <= 0) cfg_.gamesPerSeed = 1;
        if (cfg_.positionsPerGame <= 0) cfg_.positionsPerGame = 1;
        if (cfg_.sampleStride <= 0) cfg_.sampleStride = 1;
        if (cfg_.moveSample <= 0) cfg_.moveSample = 1;
    }

    bool run() {
        auto seeds = loadSeeds();
        if (seeds.empty()) {
            std::cout << "info string gen no FEN seeds found in " << cfg_.fenPath << std::endl;
            return false;
        }

        out_.open(cfg_.outputPath, std::ios::out | std::ios::trunc);
        if (!out_) {
            std::cout << "info string gen unable to open " << cfg_.outputPath << std::endl;
            return false;
        }

        std::cout << "info string gen loaded " << seeds.size()
                  << " opening seeds from " << cfg_.fenPath << std::endl;

        std::atomic<std::size_t> next{0};
        int threadCount = std::max(1, cfg_.threads);
        std::vector<std::thread> threads;
        threads.reserve(threadCount);

        for (int t = 0; t < threadCount; ++t) {
            threads.emplace_back([&, t]() {
                ThisBot evaluator(params_);
                std::mt19937 rng(cfg_.seed + static_cast<unsigned>(t));

                while (true) {
                    std::size_t idx = next.fetch_add(1);
                    if (idx >= seeds.size()) break;

                    for (int g = 0; g < cfg_.gamesPerSeed; ++g) {
                        auto samples = playGame(seeds[idx], evaluator, rng);
                        writeSamples(samples);
                    }
                }
            });
        }

        for (auto& th : threads) th.join();

        std::cout << "info string gen wrote " << totalWritten_
                  << " positions to " << cfg_.outputPath << std::endl;
        return true;
    }

private:
    Move pickMove(Board& board, ThisBot& evaluator, std::mt19937& rng) const {
        MoveList moves;
        board.generateLegalMoves(moves);
        if (moves.size() == 0) return Move();

        std::vector<std::pair<int, Move>> scored;
        scored.reserve(moves.size());
        for (int i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            auto undo = board.makeMove(m);
            int score = -evaluator.evaluateForTuning(board);
            board.unmakeMove(m, undo);
            scored.push_back({score, m});
        }

        std::shuffle(scored.begin(), scored.end(), rng);
        int sample = std::min(cfg_.moveSample, static_cast<int>(scored.size()));
        std::pair<int, Move> best = scored[0];
        for (int i = 1; i < sample; ++i) {
            if (scored[i].first > best.first) best = scored[i];
        }
        return best.second;
    }

    int labelPosition(ThisBot& evaluator, Board& board) const {
        int score = 0;
        if (cfg_.useSearchLabels) {
            Move mv = evaluator.think(board, cfg_.labelMoveTimeMs, cfg_.labelMoveTimeMs, cfg_.labelDepth, cfg_.labelMaxNodes);
            (void)mv;
            score = evaluator.lastScore();
        } else {
            score = evaluator.evaluateForTuning(board);
        }
        if (score > cfg_.evalClip) score = cfg_.evalClip;
        else if (score < -cfg_.evalClip) score = -cfg_.evalClip;
        return score;
    }

    std::vector<PositionEvalSample> playGame(const std::string& seedFen, ThisBot& evaluator, std::mt19937& rng) const {
        Board board;
        board.setFEN(seedFen);

        // Apply random opening moves
        for (int i = 0; i < cfg_.randomPlies; ++i) {
            MoveList moves;
            board.generateLegalMoves(moves);
            if (moves.size() == 0) break;
            std::uniform_int_distribution<int> dist(0, moves.size() - 1);
            Move mv = moves[dist(rng)];
            board.makeMove(mv);
            if (board.isCheckmate() || board.isStalemate()) break;
        }

        std::vector<PositionEvalSample> collected;
        collected.reserve(cfg_.positionsPerGame * 2);

        // FIX: Randomize the starting offset. 
        // If sampleStride is 2, this will be 0 or 1.
        // This ensures we capture both White-to-move and Black-to-move positions across different games.
        std::uniform_int_distribution<int> offsetDist(0, std::max(1, cfg_.sampleStride) - 1);
        int startOffset = offsetDist(rng);

        for (int ply = 0; ply < cfg_.maxPlies; ++ply) {
            // Apply the offset to the modulo check
            if (((ply + startOffset) % cfg_.sampleStride) == 0 && !board.inCheck()) {
                collected.push_back({board.toFEN(), labelPosition(evaluator, board)});
            }

            Move mv = pickMove(board, evaluator, rng);
            if (mv.isNull()) break;
            board.makeMove(mv);

            if (board.isCheckmate() || board.isStalemate() || board.halfmove_ >= 100) break;
        }

        if (collected.size() > static_cast<std::size_t>(cfg_.positionsPerGame)) {
            std::shuffle(collected.begin(), collected.end(), rng);
            collected.resize(cfg_.positionsPerGame);
        }

        return collected;
    }

    std::vector<std::string> loadSeeds() const {
        std::ifstream in(cfg_.fenPath);
        if (!in) return {};

        std::vector<std::string> seeds;
        std::string line;
        while (std::getline(in, line)) {
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            auto end = line.find_last_not_of(" \t\r\n");
            seeds.push_back(line.substr(start, end - start + 1));
        }
        return seeds;
    }

    void writeSamples(const std::vector<PositionEvalSample>& samples) {
        if (samples.empty()) return;
        std::lock_guard<std::mutex> lock(outMutex_);
        for (const auto& s : samples) {
            out_ << s.fen << " | " << s.eval << "\n";
        }
        totalWritten_ += samples.size();
    }

    PositionGenConfig cfg_;
    std::shared_ptr<EvalParams> params_;
    std::mutex outMutex_;
    std::ofstream out_;
    std::uint64_t totalWritten_ = 0;
};

//============================================================================
// Texel tuner for PSTs
//============================================================================
struct LabeledPosition {
    std::string fen;
    double result; // 1.0 = white win, 0.5 = draw, 0.0 = black win
};

struct TexelConfig {
    int iterations = 100;
    double learningRate = 2.0;
    double K = 1.13; // Sigmoid scaling factor (will be optimized)
    int positionsPerGame = 8; // Sample positions per game
    int numGames = 100; // Games for position generation
    int moveTimeMs = 20;
    int searchDepth = 4;
    int maxPlies = 150;
    unsigned seed = 12345;
    bool optimizeK = true;
};

class TexelTuner {
public:
    TexelTuner(const TexelConfig& cfg, std::shared_ptr<EvalParams> params)
        : cfg_(cfg), params_(params), rng_(cfg.seed) {}

    void generatePositions() {
        std::cout << "info string texel generating " << cfg_.numGames << " games for training data" << std::endl;
        
        std::mutex posMutex;
        std::atomic<int> gamesCompleted{0};
        
        auto playGame = [&](unsigned gameSeed) {
            Board board;
            board.reset();
            
            ThisBot white(params_);
            ThisBot black(params_);
            
            std::mt19937 localRng(gameSeed);
            std::vector<std::string> gameFens;
            
            // Random opening moves for variety
            for (int i = 0; i < 4; ++i) {
                MoveList moves;
                board.generateLegalMoves(moves);
                if (moves.size() == 0) break;
                std::uniform_int_distribution<int> dist(0, moves.size() - 1);
                board.makeMove(moves[dist(localRng)]);
                if (board.isCheckmate() || board.isStalemate()) break;
            }
            
            double result = 0.5;
            for (int ply = 0; ply < cfg_.maxPlies; ++ply) {
                MoveList legals;
                board.generateLegalMoves(legals);
                if (legals.size() == 0) {
                    result = board.inCheck() ? (board.sideToMove_ == WHITE ? 0.0 : 1.0) : 0.5;
                    break;
                }
                
                // Store position (skip very early and checks for cleaner data)
                if (ply >= 8 && !board.inCheck()) {
                    gameFens.push_back(board.toFEN());
                }
                
                ThisBot& engine = board.sideToMove_ == WHITE ? white : black;
                Move mv = engine.think(board, cfg_.moveTimeMs, cfg_.moveTimeMs, cfg_.searchDepth);
                if (mv.isNull()) { result = 0.5; break; }
                
                board.makeMove(mv);
                
                if (board.isCheckmate()) {
                    result = board.sideToMove_ == WHITE ? 0.0 : 1.0;
                    break;
                }
                if (board.isStalemate() || board.halfmove_ >= 100) {
                    result = 0.5;
                    break;
                }
            }
            
            // Sample positions from this game
            if (!gameFens.empty()) {
                std::lock_guard<std::mutex> lock(posMutex);
                std::shuffle(gameFens.begin(), gameFens.end(), localRng);
                int toAdd = std::min(cfg_.positionsPerGame, static_cast<int>(gameFens.size()));
                for (int i = 0; i < toAdd; ++i) {
                    positions_.push_back({gameFens[i], result});
                }
            }
            
            int completed = ++gamesCompleted;
            if (completed % 10 == 0) {
                std::cout << "info string texel games " << completed << "/" << cfg_.numGames << std::endl;
            }
        };
        
        std::vector<std::thread> threads;
        int numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
        
        for (int g = 0; g < cfg_.numGames; ++g) {
            if (static_cast<int>(threads.size()) >= numThreads) {
                for (auto& t : threads) t.join();
                threads.clear();
            }
            threads.emplace_back(playGame, cfg_.seed + g);
        }
        for (auto& t : threads) t.join();
        
        std::cout << "info string texel collected " << positions_.size() << " positions" << std::endl;
    }

    double computeOptimalK() {
        double bestK = 1.0;
        double bestError = 1e9;
        
        for (double k = 0.5; k <= 2.0; k += 0.05) {
            cfg_.K = k;
            double err = computeError();
            if (err < bestError) {
                bestError = err;
                bestK = k;
            }
        }
        
        // Fine-tune
        for (double k = bestK - 0.05; k <= bestK + 0.05; k += 0.01) {
            cfg_.K = k;
            double err = computeError();
            if (err < bestError) {
                bestError = err;
                bestK = k;
            }
        }
        
        cfg_.K = bestK;
        std::cout << "info string texel optimal K = " << bestK << " (error = " << bestError << ")" << std::endl;
        return bestK;
    }

    void tune() {
        if (positions_.empty()) {
            std::cout << "info string texel error: no positions" << std::endl;
            return;
        }
        
        if (cfg_.optimizeK) {
            computeOptimalK();
        }
        
        double baseError = computeError();
        std::cout << "info string texel initial error: " << baseError << std::endl;
        
        // Collect PST references
        std::vector<std::pair<double*, std::pair<int,int>>> pstRefs; // ptr, (pt, sq)
        for (int pt = 0; pt < 6; ++pt) {
            for (int sq = 0; sq < 64; ++sq) {
                pstRefs.push_back(std::make_pair(&params_->pst[pt][sq], std::make_pair(pt, sq)));
            }
        }
        for (int sq = 0; sq < 64; ++sq) {
            pstRefs.push_back(std::make_pair(&params_->kingEndgame[sq], std::make_pair(6, sq)));
        }
        
        const double epsilon = 0.5;
        
        for (int iter = 0; iter < cfg_.iterations; ++iter) {
            std::vector<double> gradients(pstRefs.size(), 0.0);
            
            // Compute numerical gradients
            for (size_t i = 0; i < pstRefs.size(); ++i) {
                double original = *pstRefs[i].first;
                
                *pstRefs[i].first = original + epsilon;
                double errPlus = computeError();
                
                *pstRefs[i].first = original - epsilon;
                double errMinus = computeError();
                
                *pstRefs[i].first = original;
                
                gradients[i] = (errPlus - errMinus) / (2.0 * epsilon);
            }
            
            // Apply gradients with adaptive learning rate
            double lr = cfg_.learningRate / (1.0 + iter * 0.01);
            for (size_t i = 0; i < pstRefs.size(); ++i) {
                double delta = -lr * gradients[i];
                delta = std::max(-5.0, std::min(5.0, delta)); // Clamp update
                *pstRefs[i].first += delta;
                
                // Clamp to reasonable PST bounds
                *pstRefs[i].first = std::max(-150.0, std::min(150.0, *pstRefs[i].first));
            }
            
            double newError = computeError();
            
            if ((iter + 1) % 10 == 0 || iter == 0) {
                std::cout << "info string texel iter " << (iter + 1) 
                          << "/" << cfg_.iterations 
                          << " error " << newError 
                          << " lr " << lr << std::endl;
            }
            
            // Early stopping if not improving
            if (iter > 20 && newError >= baseError * 0.999) {
                std::cout << "info string texel early stop at iter " << (iter + 1) << std::endl;
                break;
            }
            baseError = newError;
        }
        
        std::cout << "info string texel final error: " << computeError() << std::endl;
    }

private:
    TexelConfig cfg_;
    std::shared_ptr<EvalParams> params_;
    std::vector<LabeledPosition> positions_;
    std::mt19937 rng_;

    double sigmoid(double eval) const {
        return 1.0 / (1.0 + std::pow(10.0, -cfg_.K * eval / 400.0));
    }

    double computeError() const {
        if (positions_.empty()) return 1.0;
        
        // Create an engine with current parameters for evaluation
        ThisBot evalEngine(params_);
        double totalError = 0.0;
        
        for (const auto& pos : positions_) {
            Board board;
            board.setFEN(pos.fen);
            
            // Use the REAL evaluation function
            int eval = evalEngine.evaluateForTuning(board);
            
            double predicted = sigmoid(eval);
            double error = pos.result - predicted;
            totalError += error * error;
        }
        
        return totalError / positions_.size();
    }
};
} // namespace chess
