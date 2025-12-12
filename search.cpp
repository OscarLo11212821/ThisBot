namespace chess {

static inline int scoreToTT(int score, int ply) {
    constexpr int MATE = 20000;
    if (score > MATE - 100) return score + ply;
    if (score < -MATE + 100) return score - ply;
    return score;
}

static inline int scoreFromTT(int score, int ply) {
    constexpr int MATE = 20000;
    if (score > MATE - 100) return score - ply;
    if (score < -MATE + 100) return score + ply;
    return score;
}

Move ThisBot::think(Board& board, int softMs, int hardMs, int maxDepth, std::uint64_t maxNodes) {
    timeSoftMs_ = softMs;
    timeHardMs_ = hardMs;
    maxNodes_ = maxNodes;
    timeUp_ = false;
    stopFlag_ = false;
    nodes_ = 0;
    selDepth_ = 0;
    start_ = std::chrono::steady_clock::now();
    bestRoot_ = Move();

    MoveList rootMoves;
    board.generateLegalMoves(rootMoves);
    if (rootMoves.size() == 0) return Move();

    if (prevRoot_.isNull()) prevRoot_ = rootMoves[0];
    bestRoot_ = prevRoot_;

    int lastCompletedDepth = 0;
    int lastScore = 0;

    for (int depth = 1; depth <= maxDepth; ++depth) {
        for (auto& r : history_) for (int& v : r) v >>= 1;
        selDepth_ = 0;

        int alpha = -INF, beta = INF;
        if (depth >= 5) { alpha = prevScore_ - 50; beta = prevScore_ + 50; }
        int score = search(board, depth, alpha, beta, 0, Move());
        if (!timeUp_ && (score <= alpha || score >= beta)) {
            alpha = -INF; beta = INF;
            score = search(board, depth, alpha, beta, 0, Move());
        }
        if (timeUp_) break;
        prevRoot_ = bestRoot_;
        prevScore_ = score;
        lastCompletedDepth = depth;
        lastScore = score;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count();
        auto nps = elapsed > 0 ? (nodes_ * 1000LL) / elapsed : 0;
        
        std::string scoreStr;
        if (std::abs(score) >= MATE - 100) {
            int matePly = MATE - std::abs(score);
            int mateIn = (matePly + 1) / 2;
            if (score > 0) scoreStr = "mate " + std::to_string(mateIn);
            else scoreStr = "mate -" + std::to_string(mateIn);
        } else {
            scoreStr = "cp " + std::to_string(score);
        }
        
        std::cout << "info depth " << depth
                  << " seldepth " << selDepth_
                  << " score " << scoreStr
                  << " time " << elapsed
                  << " nodes " << nodes_
                  << " nps " << nps
                  << " hashfull " << getHashFull()
                  << " pv " << board.moveToUCI(bestRoot_)
                  << std::endl;

        if (timeSoftMs_ > 0 && elapsed >= timeSoftMs_) break;
        
        if (std::abs(score) > 19000) break;
    }

    Move result = timeUp_ ? prevRoot_ : bestRoot_;
    if (result.isNull()) result = rootMoves[0];
    return result;
}


int ThisBot::quiescence(Board& board, int alpha, int beta, int qDepth) {
    if (stopFlag_) { timeUp_ = true; return 0; }
    if (timeUp_ || qDepth > 10) return evaluate(board);

    const auto& p = *params_;
    bool lowMaterial = isLowMaterialEnding(board);
    int standPat = evaluate(board);
    if (standPat >= beta) return beta;
    if (alpha < standPat) alpha = standPat;

    MoveList moves;
    board.generateLegalMoves(moves);

    std::vector<std::pair<int, Move>> scoredMoves;
    for (int i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        bool isCapture = !board.isEmpty(m.to()) || m.type() == MT_EN_PASSANT;
        bool isPromo = m.type() == MT_PROMOTION;
        if (!isCapture && !isPromo) continue;

        int score = 0;
        if (isCapture) {
            PieceType captured = m.type() == MT_EN_PASSANT ? PAWN : board.pieceAt(m.to());
            score = static_cast<int>(p.pieceValues[captured] * 10 - p.pieceValues[board.pieceAt(m.from())]);
        }
        if (isPromo) score += 8000;
        scoredMoves.push_back({score, m});
    }

    if (scoredMoves.empty()) return alpha;

    for (size_t i = 1; i < scoredMoves.size(); ++i) {
        auto temp = scoredMoves[i];
        int j = static_cast<int>(i) - 1;
        while (j >= 0 && scoredMoves[j].first < temp.first) {
            scoredMoves[j + 1] = scoredMoves[j];
            --j;
        }
        scoredMoves[j + 1] = temp;
    }

    for (const auto& [moveScore, m] : scoredMoves) {
        bool isCapture = !board.isEmpty(m.to()) || m.type() == MT_EN_PASSANT;
        bool isPromo = m.type() == MT_PROMOTION;

        if (!isPromo) {
            PieceType captured = m.type() == MT_EN_PASSANT ? PAWN : board.pieceAt(m.to());
            int margin = (captured == PAWN || lowMaterial) ? 0 : 200;
            if (standPat + static_cast<int>(p.pieceValues[captured]) + margin < alpha) continue;
        }

        if (!isPromo && isCapture) {
            int attacker = static_cast<int>(p.pieceValues[board.pieceAt(m.from())]);
            PieceType captured = m.type() == MT_EN_PASSANT ? PAWN : board.pieceAt(m.to());
            int victim = static_cast<int>(p.pieceValues[captured]);
            if (attacker - victim > 80 && attacker > victim && see(board, m) < 0)
                continue;
        }

        auto undo = board.makeMove(m);
        int score = -quiescence(board, -beta, -alpha, qDepth + 1);
        board.unmakeMove(m, undo);

        if (timeUp_) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int ThisBot::search(Board& board, int depth, int alpha, int beta, int ply, Move prevMove) {
    if (ply >= 100) return evaluate(board);
    
    if (ply > selDepth_) selDepth_ = ply;

    if ((++nodes_ & 2047) == 0) {
        if (stopFlag_) {
            timeUp_ = true;
            return 0;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count();
        if (timeHardMs_ > 0 && elapsed >= timeHardMs_) {
            timeUp_ = true;
            return 0;
        }
        if (maxNodes_ > 0 && static_cast<std::uint64_t>(nodes_) >= maxNodes_) {
            timeUp_ = true;
            return 0;
        }
    }

    bool isRoot = ply == 0;
    bool pvNode = (beta - alpha) > 1;
    int origAlpha = alpha;
    const auto& p = *params_;

    if (board.isDraw()) return 0;

    // Check extension BEFORE TT probe
    bool inCheck = board.inCheck();
    if (inCheck) depth++;

    if (depth <= 0) return quiescence(board, alpha, beta, 0);

    std::uint64_t key = hash(board);
    TTEntry& tt = tt_[key & (tt_.size() - 1)];
    Move ttMove;
    
    // TT lookup - restrict cutoffs at PV nodes
    if (tt.key == key) {
        ttMove = tt.move;
        if (!isRoot && tt.depth >= depth) {
            int ttScore = scoreFromTT(tt.score, ply);
            if (tt.flag == 1) {
                // Exact score - can use at PV nodes
                return ttScore;
            }
            if (!pvNode) {
                // Bounds only usable at non-PV nodes
                if (tt.flag == 2 && ttScore >= beta) return ttScore;
                if (tt.flag == 3 && ttScore <= alpha) return ttScore;
            }
        }
    }

    int staticEval = inCheck ? -MATE : evaluate(board);
    bool lateEg = isLowMaterialEnding(board);

    if (!pvNode && !inCheck && depth <= 6 && staticEval - 90 * depth >= beta)
        return staticEval;

    // Null-move pruning
    if (!inCheck && !pvNode && depth >= 3 && staticEval >= beta && hasNonPawnMaterial(board)) {
        auto nu = board.makeNullMove();
        int R = 3 + depth / 4;
        int score = -search(board, depth - R, -beta, -beta + 1, ply + 1, Move());
        board.unmakeNullMove(nu);
        if (timeUp_) return 0;
        if (score >= beta) return score;
    }

    // PV TT warmup
    if (depth >= 6 && pvNode && ttMove.isNull()) {
        search(board, depth - 3, alpha, beta, ply, prevMove);
        if (tt.key == key) ttMove = tt.move;
    }

    MoveList moves;
    board.generateLegalMoves(moves);
    int moveCount = moves.size();
    if (moveCount == 0) {
        if (!inCheck) return 0;
        int mateScore = -MATE + ply;
        return mateScore;
    }

    // Validate ttMove is in the legal move list
    bool ttMoveValid = false;
    if (!ttMove.isNull()) {
        for (int i = 0; i < moveCount; ++i) {
            if (moves[i] == ttMove) {
                ttMoveValid = true;
                break;
            }
        }
        if (!ttMoveValid) ttMove = Move();
    }

    if (moveCount == 1) {
        Move m = moves[0];
        bool isQuiet = board.isEmpty(m.to()) && m.type() != MT_PROMOTION && m.type() != MT_EN_PASSANT;

        auto u = board.makeMove(m);
        int score = -search(board, depth - 1, -beta, -alpha, ply + 1, m);
        board.unmakeMove(m, u);
        if (timeUp_) return 0;

        int bestScore = score;
        Move bestMove = m;

        if (score > alpha) {
            alpha = score;
            if (isRoot) bestRoot_ = m;
            if (score >= beta && isQuiet) {
                history_[m.from()][m.to()] += depth * depth;
                if (ply < 128 && !(m == killers_[ply][0])) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = m;
                }
                if (!prevMove.isNull()) {
                    counterMoves_[prevMove.from()][prevMove.to()] = m;
                }
            }
        }
        
        if (tt.key != key || depth >= tt.depth) {
            tt.key = key; 
            tt.depth = depth; 
            tt.score = scoreToTT(bestScore, ply);
            tt.move = bestMove;
            tt.flag = bestScore <= origAlpha ? 3 : bestScore >= beta ? 2 : 1;
        }
        return bestScore;
    }

    std::vector<int> scores(moveCount, 0);
    for (int i = 0; i < moveCount; ++i) {
        Move m = moves[i];
        if (m == ttMove) { scores[i] = 2000000; continue; }

        bool isCapture = !board.isEmpty(m.to()) || m.type() == MT_EN_PASSANT;
        PieceType moving = board.pieceAt(m.from());
        PieceType captured = isCapture ? (m.type() == MT_EN_PASSANT ? PAWN : board.pieceAt(m.to())) : NO_PIECE;

        if (isCapture) {
            scores[i] = 1000000 + mvvLva(moving, captured);
        } else if (m.type() == MT_PROMOTION) {
            scores[i] = 900000 + static_cast<int>(p.pieceValues[m.promo()]);
        } else if (ply < 128 && m == killers_[ply][0]) {
            scores[i] = 800000;
        } else if (!prevMove.isNull() && m == counterMoves_[prevMove.from()][prevMove.to()]) {
            scores[i] = 750000;
        } else if (ply < 128 && m == killers_[ply][1]) {
            scores[i] = 700000;
        } else {
            scores[i] = history_[m.from()][m.to()];
        }
    }

    for (int i = 1; i < moveCount; ++i) {
        int s = scores[i];
        Move m = moves[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < s) {
            scores[j + 1] = scores[j];
            moves[j + 1] = moves[j];
            --j;
        }
        scores[j + 1] = s;
        moves[j + 1] = m;
    }

    int bestScore = -INF;
    Move bestMove = moves[0];
    int movesSearched = 0;

    int lmrIdx = std::min(depth, 63);
    const auto& lmrRow = pc.lmr[lmrIdx];

    // Track quiet moves tried for history penalty
    std::vector<Move> quietsTried;

    for (int i = 0; i < moveCount; ++i) {
        Move m = moves[i];
        bool isCapture = !board.isEmpty(m.to()) || m.type() == MT_EN_PASSANT;
        bool isPromotion = m.type() == MT_PROMOTION;
        bool isQuiet = !isCapture && !isPromotion;

        // SEE-based capture pruning
        if (m.type() != MT_EN_PASSANT && !board.isEmpty(m.to()) && movesSearched > 0 && !isPromotion) {
            PieceType attacker = board.pieceAt(m.from());
            PieceType victim = board.pieceAt(m.to());
            if (p.pieceValues[attacker] - p.pieceValues[victim] > 80 && depth <= 6 && movesSearched >= 2) {
                int s = see(board, m);
                if (s < -100) continue;
            }
        }

        // Quiet pruning
        if (!lateEg && !pvNode && !inCheck && depth <= 7 && staticEval + 100 * depth < alpha && movesSearched > 0 && isQuiet && bestScore > -19000) continue;
        if (!lateEg && !pvNode && !inCheck && depth <= 5 && movesSearched >= 4 + depth * depth / 2 && isQuiet && bestScore > -19000) continue;

        auto undo = board.makeMove(m);
        bool givesCheck = board.inCheck();
        int score;

        if (movesSearched >= 3 && depth >= 3 && isQuiet && !inCheck && !givesCheck) {
            int reduction = lmrRow[std::min(movesSearched, 63)];
            reduction = std::min(reduction, depth - 2);
            if (pvNode) reduction = std::max(0, reduction - 1);
            if (lateEg) reduction = std::max(0, reduction - 1);
            score = -search(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, m);
            if (score > alpha) score = -search(board, depth - 1, -beta, -alpha, ply + 1, m);
        } else if (movesSearched == 0) {
            score = -search(board, depth - 1, -beta, -alpha, ply + 1, m);
        } else {
            score = -search(board, depth - 1, -alpha - 1, -alpha, ply + 1, m);
            if (score > alpha && score < beta) score = -search(board, depth - 1, -beta, -alpha, ply + 1, m);
        }

        board.unmakeMove(m, undo);
        movesSearched++;

        if (timeUp_) return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            if (score > alpha) {
                alpha = score;
                if (isRoot) bestRoot_ = m;
                if (alpha >= beta) {
                    if (isQuiet) {
                        history_[m.from()][m.to()] += depth * depth;
                        // Penalize all quiets tried before the cutoff move
                        for (const Move& q : quietsTried) {
                            history_[q.from()][q.to()] -= depth * depth;
                        }
                        if (ply < 128) {
                            if (!(m == killers_[ply][0])) {
                                killers_[ply][1] = killers_[ply][0];
                                killers_[ply][0] = m;
                            }
                        }
                        if (!prevMove.isNull()) {
                            counterMoves_[prevMove.from()][prevMove.to()] = m;
                        }
                    }
                    break;
                }
            }
        }

        // Track quiets tried, don't penalize here
        if (isQuiet) {
            quietsTried.push_back(m);
        }
    }

    // TT storage with mate score adjustment
    if (tt.key != key || depth >= tt.depth) {
        tt.key = key;
        tt.depth = depth;
        tt.score = scoreToTT(bestScore, ply);
        tt.move = bestMove;
        tt.flag = bestScore <= origAlpha ? 3 : (bestScore >= beta ? 2 : 1);
    }

    return bestScore;
}

} // namespace chess
