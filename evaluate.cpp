namespace chess {
struct EvalParams {
    std::array<double, 7> pieceValues { 99.1802,315.696,379.541,541.839,1045.78,20000.2,0 };
    std::array<double, 7> mobilityBonus { 0.558513,6.02874,6.76868,4.99854,5.36574,-1.17361,1.06308 };
    std::array<double, 9> attackWeight { -1.38263,3.8379,39.6664,51.0186,85.7022,74.4797,64.7077,86.6042,116.352 };
    std::array<double, 7> pieceAttackValue { -0.127949,9.01816,17.0672,41.3365,62.978,-0.344534,-1.83642 };
    std::array<double, 8> passedPawnBonus { -2.11314,-2.56899,12.6536,10.6128,44.3603,85.3953,131.088,2.05708 };
    std::array<double, 2> knightOutpostBonus { 15.1689,11.0465 };
    std::array<double, 2> rookFileBonus { 16.7742,10.5363 };
    std::array<double, 4> developmentWeights { 23.0955,12.6867,8.71504,36.1943 };
    std::array<double, 3> kingShieldValues { 10.5231,20.2514,16.2529 };
    std::array<double, 2> rookSeventhBonus { 25.0444,39.8164 };
    std::array<double, 2> badBishopPenalty { 12.2557,3.85626 };

    // PST tables - [piece_type][square] for pawn(0), knight(1), bishop(2), rook(3), queen(4), king_mg(5)
    std::array<std::array<double, 64>, 6> pst;
    // King endgame PST
    std::array<double, 64> kingEndgame;

    static EvalParams defaults() { 
        EvalParams p;
        
        // Default PST values (from original static constexpr)
        // Pawn PST
        p.pst[0] = {{
            0,   0,   0,   0,   0,   0,   0,   0,
            5,   5,   5,  -20, -20,  5,   5,   5,
            5,   5,   5,   5,   5,   5,   5,   5,
            5,  10,  20,  35,  35,  20,  10,  5,
            10, 20,  25,  30,  30,  25,  20,  10,
            20, 30,  35,  55,  55,  35,  30,  20,
            50, 60,  70,  80,  80,  70,  60,  50,
            0,   0,   0,   0,   0,   0,   0,   0
        }};
        
        // Knight PST
        p.pst[1] = {{
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,   5,   5,   0, -20,-40,
            -30,  5,  20,  25,  25,  20,  5, -30,
            -30,  5,  25,  30,  30,  25,  5, -30,
            -30,  0,  25,  30,  30,  25,  0, -30,
            -30,  5,  15,  20,  20,  15,  5, -30,
            -40,-20,  0,   5,   5,   0, -20,-40,
            -50,-40,-20,-30,-30,-20,-40,-50
        }};
        
        // Bishop PST
        p.pst[2] = {{
            -20,-10,-40,-10,-10,-40,-10,-20,
            -10,  25,  10,  10,  10,  10,  25,-10,
            -10,  10,  15,  15,  15,  15,  10,-10,
            -10,  10,  15,  20,  20,  15,  10,-10,
            -10,  10,  15,  20,  20,  15,  10,-10,
            -10,  15,  15,  15,  15,  15,  15,-10,
            -10,  25,  10,  10,  10,  10,  25,-10,
            -20,-10,-10,-10,-10,-10,-10,-20
        }};
        
        // Rook PST
        p.pst[3] = {{
            5,   5,   10,  10,  10,  10,  5,   5,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            20,  20,  20,  20,  20,  20,  20,  20,
            0,   5,   5,   5,   5,   5,   5,   0
        }};
        
        // Queen PST
        p.pst[4] = {{
            -20,-10,-10, -5,  -5, -10,-10,-20,
            -10,  0,   0,   0,   0,   0,   0, -10,
            -10,  0,   5,   5,   5,   5,   0, -10,
            -5,   0,   5,   10,  10,  5,   0,  -5,
            -5,   0,   5,   10,  10,  5,   0,  -5,
            -10,  0,   5,   5,   5,   5,   0, -10,
            -10,  0,   0,   0,   0,   0,   0, -10,
            -20,-10,-10, -5,  -5, -10,-10,-20
        }};
        
        // King middlegame PST
        p.pst[5] = {{
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -20,-30,-30,-40,-40,-30,-30,-20,
            10,  10, -10, -20, -20, -10,  10,  10,
            10,  25,  30,  -15,  0,  -15,  30,  25
        }};
        
        // King endgame PST
        p.kingEndgame = {{
            -50,-40,-30,-20,-20,-30,-40,-50,
            -30,-20,-10,  0,  0, -10,-20,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-30,  0,  0,  0,  0,-30,-30,
            -50,-30,-30,-30,-30,-30,-30,-50
        }};
        
        return p;
    }

    static EvalParams minBounds() {
        EvalParams p;
        p.pieceValues = { 50, 200, 200, 250, 400, 15000, 0 };
        p.mobilityBonus = { -50, -50, -50, -50, -50, -50, -50 };
        p.attackWeight = { -200, -200, -200, -200, -200, -200, -200, -200, -200 };
        p.pieceAttackValue = { -50, -50, -50, -50, -50, -50, -50 };
        p.passedPawnBonus = { -50, -50, -50, -50, -50, -50, -50, -50 };
        p.knightOutpostBonus = { -50, -50 };
        p.rookFileBonus = { -50, -50 };
        p.developmentWeights = { -50, -50, -50, -50 };
        p.kingShieldValues = { -50, -50, -50 };
        p.rookSeventhBonus = { -20, -20 };
        p.badBishopPenalty = { 0, 0 };
        
        // PST bounds: allow reasonable range for piece placement
        for (int pt = 0; pt < 6; ++pt) {
            for (int sq = 0; sq < 64; ++sq) {
                p.pst[pt][sq] = -100.0;
            }
        }
        for (int sq = 0; sq < 64; ++sq) {
            p.kingEndgame[sq] = -100.0;
        }
        
        return p;
    }

    static EvalParams maxBounds() {
        EvalParams p;
        p.pieceValues = { 400, 700, 800, 1200, 2000, 30000, 0 };
        p.mobilityBonus = { 100, 200, 200, 100, 100, 100, 100 };
        p.attackWeight = { 200, 200, 200, 200, 200, 200, 200, 200, 200 };
        p.pieceAttackValue = { 150, 150, 150, 150, 150, 150, 150 };
        p.passedPawnBonus = { 300, 300, 300, 300, 300, 300, 300, 300 };
        p.knightOutpostBonus = { 200, 200 };
        p.rookFileBonus = { 200, 200 };
        p.developmentWeights = { 150, 150, 150, 150 };
        p.kingShieldValues = { 200, 200, 200 };
        p.rookSeventhBonus = { 80, 100 };
        p.badBishopPenalty = { 60, 60 };
        
        // PST bounds
        for (int pt = 0; pt < 6; ++pt) {
            for (int sq = 0; sq < 64; ++sq) {
                p.pst[pt][sq] = 100.0;
            }
        }
        for (int sq = 0; sq < 64; ++sq) {
            p.kingEndgame[sq] = 100.0;
        }
        
        return p;
    }
};

//======================================================================
// ThisBot
//======================================================================
class ThisBot {
public:
    ThisBot(std::shared_ptr<EvalParams> params = nullptr);
    Move think(Board& board, int softMs = 200, int hardMs = 200, int maxDepth = 32, std::uint64_t maxNodes = 0);
    void stop() { stopFlag_ = true; }
    void setHashSize(size_t mbSize);
    size_t getHashSize() const { return tt_.size() * sizeof(TTEntry) / (1024 * 1024); }
    int getSelDepth() const { return selDepth_; }
    int getHashFull() const;
    void setEvalParams(std::shared_ptr<EvalParams> params) { params_ = std::move(params); }
    const EvalParams& evalParams() const { return *params_; }
    int lastScore() const { return prevScore_; }
    int searchScore(Board& board, int depth, int hardMs = 0, std::uint64_t maxNodes = 0);
    int scoreMoveSearch(Board& board, Move m, int depth, int hardMs = 0, std::uint64_t maxNodes = 0);

    int evaluateForTuning(Board& board) { return evaluate(board); }

private:
    using Bitboard = std::uint64_t;

    static constexpr int INF = 30000;
    static constexpr int MATE = 20000;

    // Phase constants remain fixed (used for interpolation)
    static constexpr int PIECE_PHASE[7]           = { 0, 1, 1, 2, 4, 0, 0 };

    // Piece-square tables (0 = pawn, 1 = knight, 2 = bishop, 3 = rook, 4 = queen, 5 = king MG)
    static constexpr int PST[6][64] = {
        {
            0,   0,   0,   0,   0,   0,   0,   0,
            5,   5,   5,  -20, -20,  5,   5,   5,
            5,   5,   5,   5,   5,   5,   5,   5,
            5,  10,  20,  35,  35,  20,  10,  5,
            10, 20,  25,  30,  30,  25,  20,  10,
            20, 30,  35,  55,  55,  35,  30,  20,
            50, 60,  70,  80,  80,  70,  60,  50,
            0,   0,   0,   0,   0,   0,   0,   0
        },
        {
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,   5,   5,   0, -20,-40,
            -30,  5,  20,  25,  25,  20,  5, -30,
            -30,  5,  25,  30,  30,  25,  5, -30,
            -30,  0,  25,  30,  30,  25,  0, -30,
            -30,  5,  15,  20,  20,  15,  5, -30,
            -40,-20,  0,   5,   5,   0, -20,-40,
            -50,-40,-20,-30,-30,-20,-40,-50
        },
        {
            -20,-10,-40,-10,-10,-40,-10,-20,
            -10,  25,  10,  10,  10,  10,  25,-10,
            -10,  10,  15,  15,  15,  15,  10,-10,
            -10,  10,  15,  20,  20,  15,  10,-10,
            -10,  10,  15,  20,  20,  15,  10,-10,
            -10,  15,  15,  15,  15,  15,  15,-10,
            -10,  25,  10,  10,  10,  10,  25,-10,
            -20,-10,-10,-10,-10,-10,-10,-20
        },
        {
            5,   5,   10,  10,  10,  10,  5,   5,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,
            20,  20,  20,  20,  20,  20,  20,  20,
            0,   5,   5,   5,   5,   5,   5,   0
        },
        {
            -20,-10,-10, -5,  -5, -10,-10,-20,
            -10,  0,   0,   0,   0,   0,   0, -10,
            -10,  0,   5,   5,   5,   5,   0, -10,
            -5,   0,   5,   10,  10,  5,   0,  -5,
            -5,   0,   5,   10,  10,  5,   0,  -5,
            -10,  0,   5,   5,   5,   5,   0, -10,
            -10,  0,   0,   0,   0,   0,   0, -10,
            -20,-10,-10, -5,  -5, -10,-10,-20
        },
        {
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -40,-50,-50,-60,-60,-50,-50,-40,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -20,-30,-30,-40,-40,-30,-30,-20,
            10,  10, -10, -20, -20, -10,  10,  10,
            10,  25,  30,  -15,  0,  -15,  30,  25
        }
    };

    static constexpr int KING_ENDGAME[64] = {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,  0,  0, -10,-20,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
    };

    struct TTEntry {
        std::uint64_t key = 0;
        int depth = -1;
        int score = 0;
        Move move{};
        std::uint8_t flag = 0; // 1 exact, 2 lower, 3 upper
    };

    struct Precomputed {
        std::array<Bitboard, 64> fileMasks{};
        std::array<Bitboard, 64> neighborMasks{};
        std::array<Bitboard, 64> whitePassedMasks{};
        std::array<Bitboard, 64> blackPassedMasks{};
        std::array<Bitboard, 64> kingZoneMasks{};
        std::array<std::array<int, 64>, 64> lmr{};

        Precomputed() {
            for (int sq = 0; sq < 64; ++sq) {
                int file = fileOf(sq);
                int rank = rankOf(sq);

                Bitboard fileMask = 0x0101010101010101ULL << file;
                fileMasks[sq] = fileMask;

                Bitboard adj = 0;
                if (file > 0) adj |= 0x0101010101010101ULL << (file - 1);
                if (file < 7) adj |= 0x0101010101010101ULL << (file + 1);
                neighborMasks[sq] = adj;

                Bitboard frontWhite = 0;
                for (int r = rank + 1; r < 8; ++r) frontWhite |= 0xFFULL << (r * 8);
                whitePassedMasks[sq] = (fileMask | adj) & frontWhite;

                Bitboard frontBlack = 0;
                for (int r = 0; r < rank; ++r) frontBlack |= 0xFFULL << (r * 8);
                blackPassedMasks[sq] = (fileMask | adj) & frontBlack;

                Bitboard kMask = 0;
                for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); ++f) {
                    for (int r = std::max(0, rank - 1); r <= std::min(7, rank + 2); ++r) {
                        kMask |= 1ULL << (r * 8 + f);
                    }
                }
                kingZoneMasks[sq] = kMask;
            }

            for (int d = 0; d < 64; ++d) {
                for (int m = 0; m < 64; ++m) {
                    if (d == 0 || m == 0) lmr[d][m] = 0;
                    else lmr[d][m] = 1 + static_cast<int>(std::log(static_cast<double>(d)) * std::log(static_cast<double>(m)) / 2.5);
                }
            }
        }
    };

    std::shared_ptr<EvalParams> params_;
    Precomputed pc;

    // Search state
    std::vector<TTEntry> tt_;
    std::array<std::array<int, 64>, 64> history_{};
    std::array<std::array<Move, 2>, 128> killers_{};
    std::array<std::array<Move, 64>, 64> counterMoves_{};
    Move bestRoot_{};
    Move prevRoot_{};
    int prevScore_ = 0;
    int nodes_ = 0;
    int selDepth_ = 0;
    bool timeUp_ = false;
    volatile bool stopFlag_ = false;
    std::chrono::steady_clock::time_point start_;
    int timeSoftMs_ = 200;
    int timeHardMs_ = 200;
    std::uint64_t maxNodes_ = 0;

    bool hasNonPawnMaterial(const Board& board) const {
        Bitboard npw = board.pieces_[WHITE][KNIGHT] | board.pieces_[WHITE][BISHOP] |
                       board.pieces_[WHITE][ROOK]   | board.pieces_[WHITE][QUEEN];
        Bitboard npb = board.pieces_[BLACK][KNIGHT] | board.pieces_[BLACK][BISHOP] |
                       board.pieces_[BLACK][ROOK]   | board.pieces_[BLACK][QUEEN];
        return (board.sideToMove_ == WHITE) ? (npw != 0) : (npb != 0);
    }

    bool isLowMaterialEnding(const Board& board) const {
        Bitboard nonPawn = (board.pieces_[WHITE][KNIGHT] | board.pieces_[WHITE][BISHOP] |
                            board.pieces_[WHITE][ROOK]   | board.pieces_[WHITE][QUEEN] |
                            board.pieces_[BLACK][KNIGHT] | board.pieces_[BLACK][BISHOP] |
                            board.pieces_[BLACK][ROOK]   | board.pieces_[BLACK][QUEEN]);
        int nonPawnCount = popCount(nonPawn);
        int pawnCount = popCount(board.pieces_[WHITE][PAWN] | board.pieces_[BLACK][PAWN]);
        return nonPawnCount <= 2 && pawnCount <= 6;
    }

    // Core routines
    int evaluate(const Board& board);
    int evaluateDevelopment(const Board& board, int phase);
    int evaluateKingSafety(const Board& board, bool white, Bitboard friendlyPawns, Bitboard enemyPawns, int phase, int attackers, int attackUnits);
    int search(Board& board, int depth, int alpha, int beta, int ply, Move prevMove);
    int quiescence(Board& board, int alpha, int beta, int qDepth);
    std::uint64_t hash(const Board& board) const;
    inline bool timeExceeded();
    inline int mvvLva(PieceType attacker, PieceType victim) const {
        return static_cast<int>(params_->pieceValues[victim] * 10 - params_->pieceValues[attacker]);
    }

    // Zobrist
    static inline std::array<std::array<std::array<std::uint64_t, 64>, 6>, 2> ZP{};
    static inline std::array<std::uint64_t, 16> ZCastle{};
    static inline std::array<std::uint64_t, 8> ZEp{};
    static inline std::uint64_t ZSide = 0;
    static inline std::uint64_t splitmix64(std::uint64_t& x) {
        std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
    static void initZobrist();

    int see(Board& board, Move move);
    int seeRecapture(Board& board, int square, PieceType lastVictim);
};

ThisBot::ThisBot(std::shared_ptr<EvalParams> params) :
    params_(params ? std::move(params) : std::make_shared<EvalParams>(EvalParams::defaults())),
    pc(),
    tt_(0x800000) {
    initZobrist();
    for (auto& row : history_) row.fill(0);
    for (auto& k : killers_) k = { Move(), Move() };
    for (auto& r : counterMoves_) for (auto& m : r) m = Move();
}

void ThisBot::setHashSize(size_t mbSize) {
    // Calculate number of entries that fit in mbSize megabytes
    size_t bytes = mbSize * 1024 * 1024;
    size_t numEntries = bytes / sizeof(TTEntry);
    // Round down to power of 2 for efficient masking
    size_t power = 1;
    while (power * 2 <= numEntries) power *= 2;
    tt_.clear();
    tt_.resize(power);
}

int ThisBot::getHashFull() const {
    // Sample first 1000 entries to estimate hash table usage
    int used = 0;
    size_t sample = std::min(tt_.size(), size_t(1000));
    for (size_t i = 0; i < sample; ++i) {
        if (tt_[i].key != 0) ++used;
    }
    return static_cast<int>(used * 1000 / sample);
}

bool ThisBot::timeExceeded() {
    if (stopFlag_) {
        timeUp_ = true;
        return true;
    }
    if ((nodes_ & 2047) == 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count();
        if (timeHardMs_ > 0 && elapsed >= timeHardMs_) {
            timeUp_ = true;
        }
        if (maxNodes_ > 0 && static_cast<std::uint64_t>(nodes_) >= maxNodes_) {
            timeUp_ = true;
        }
    }
    return timeUp_;
}

void ThisBot::initZobrist() {
    static std::once_flag zobristInitFlag;
    std::call_once(zobristInitFlag, []() {
        std::uint64_t seed = 0x123456789abcdefULL;
        for (int c = 0; c < 2; ++c) {
            for (int pt = 0; pt < 6; ++pt) {
                for (int sq = 0; sq < 64; ++sq) {
                    ZP[c][pt][sq] = splitmix64(seed);
                }
            }
        }
        for (int i = 0; i < 16; ++i) ZCastle[i] = splitmix64(seed);
        for (int i = 0; i < 8; ++i) ZEp[i] = splitmix64(seed);
        ZSide = splitmix64(seed);
    });
}

std::uint64_t ThisBot::hash(const Board& board) const {
    std::uint64_t h = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.mailbox_[sq] != 0xFF) {
            int pt = board.mailbox_[sq] & 7;
            int c = (board.mailbox_[sq] >> 3) & 1;
            h ^= ZP[c][pt][sq];
        }
    }
    h ^= ZCastle[board.castling_];
    if (board.epSquare_ >= 0) h ^= ZEp[board.epSquare_ & 7];
    if (board.sideToMove_ == BLACK) h ^= ZSide;
    return h;
}

int ThisBot::seeRecapture(Board& board, int square, PieceType lastVictim) {
    int minValue = INF;
    Move best;

    MoveList moves;
    board.generateLegalMoves(moves);
    for (Move m : moves) {
        if (m.to() != square) continue;
        PieceType attacker = board.pieceAt(m.from());
        int v = static_cast<int>(params_->pieceValues[attacker]);
        if (v < minValue) {
            minValue = v;
            best = m;
        }
    }

    if (best.isNull()) return 0;

    auto undo = board.makeMove(best);
    PieceType nextVictim = board.pieceAt(best.to());
    int value = static_cast<int>(params_->pieceValues[lastVictim]);
    int next = seeRecapture(board, square, nextVictim);
    board.unmakeMove(best, undo);

    return std::max(0, value - next);
}

int ThisBot::see(Board& board, Move move) {
    bool isCapture = !board.isEmpty(move.to()) || move.type() == MT_EN_PASSANT;
    if (!isCapture) return 0;

    PieceType captured = move.type() == MT_EN_PASSANT ? PAWN : board.pieceAt(move.to());
    int value = static_cast<int>(params_->pieceValues[captured]);

    PieceType mover = board.pieceAt(move.from());
    if (move.type() == MT_PROMOTION) {
        value += static_cast<int>(params_->pieceValues[move.promo()] - 100);
    }

    auto undo = board.makeMove(move);
    PieceType nextVictim = move.type() == MT_PROMOTION ? move.promo() : mover;
    int recapture = seeRecapture(board, move.to(), nextVictim);
    board.unmakeMove(move, undo);

    return value - recapture;
}


int ThisBot::evaluateDevelopment(const Board& board, int phase) {
    if (phase < 16) return 0;

    const auto& p = *params_;
    int score = 0;
    int undevelopedPenalty = static_cast<int>(p.developmentWeights[0]);
    int castleKingBonus = static_cast<int>(p.developmentWeights[1]);
    int castleQueenBonus = static_cast<int>(p.developmentWeights[2]);
    int castledBonus = static_cast<int>(p.developmentWeights[3]);

    constexpr Bitboard whiteBack = 0x00000000000000FFULL;
    constexpr Bitboard blackBack = 0xFF00000000000000ULL;

    Bitboard whiteMinor = board.pieces_[WHITE][KNIGHT] | board.pieces_[WHITE][BISHOP];
    Bitboard blackMinor = board.pieces_[BLACK][KNIGHT] | board.pieces_[BLACK][BISHOP];

    int whiteUndev = popCount(whiteMinor & whiteBack);
    int blackUndev = popCount(blackMinor & blackBack);

    score -= whiteUndev * undevelopedPenalty;
    score += blackUndev * undevelopedPenalty;

    if (board.castling_ & WHITE_OO) score += castleKingBonus;
    if (board.castling_ & WHITE_OOO) score += castleQueenBonus;
    if (board.castling_ & BLACK_OO) score -= castleKingBonus;
    if (board.castling_ & BLACK_OOO) score -= castleQueenBonus;

    int whiteKingFile = fileOf(board.kingSq(WHITE));
    int blackKingFile = fileOf(board.kingSq(BLACK));
    if (whiteKingFile <= 1 || whiteKingFile >= 6) score += castledBonus;
    if (blackKingFile <= 1 || blackKingFile >= 6) score -= castledBonus;

    return score;
}

int ThisBot::evaluateKingSafety(const Board& board, bool white, Bitboard friendlyPawns, Bitboard enemyPawns, int phase, int attackers, int attackUnits) {
    if (phase < 10 || attackers == 0) return 0;
    attackers = std::min(attackers, 8);

    const auto& p = *params_;
    int safety = 0;
    int shieldValue = static_cast<int>(p.kingShieldValues[0]);
    int semiOpenPenalty = static_cast<int>(p.kingShieldValues[1]);
    int openExtraPenalty = static_cast<int>(p.kingShieldValues[2]);

    safety -= static_cast<int>((attackUnits * p.attackWeight[attackers]) / 100);

    int kingSq = board.kingSq(white ? WHITE : BLACK);
    Bitboard kingZone = pc.kingZoneMasks[kingSq];

    Bitboard shieldZone = white ? (kingZone & 0x0000000000FFFF00ULL) : (kingZone & 0x00FFFF0000000000ULL);
    int shield = popCount(friendlyPawns & shieldZone);
    safety += shield * shieldValue;

    int kFile = fileOf(kingSq);
    for (int f = std::max(0, kFile - 1); f <= std::min(7, kFile + 1); ++f) {
        Bitboard fileMask = 0x0101010101010101ULL << f;
        if ((fileMask & friendlyPawns) == 0) {
            safety -= semiOpenPenalty;
            if ((fileMask & enemyPawns) == 0) safety -= openExtraPenalty;
        }
    }

    return safety;
}

int ThisBot::evaluate(const Board& board) {
    int mgScore = 0, egScore = 0, phase = 0;
    int whiteBishops = 0, blackBishops = 0;
    bool whiteLightBishop = false, whiteDarkBishop = false;
    bool blackLightBishop = false, blackDarkBishop = false;
    bool whitePassed = false, blackPassed = false;

    const auto& p = *params_;
    Bitboard whitePawns = board.pieces_[WHITE][PAWN];
    Bitboard blackPawns = board.pieces_[BLACK][PAWN];
    Bitboard allPieces = board.occupied_;
    constexpr Bitboard FILE_A = 0x0101010101010101ULL;
    constexpr Bitboard FILE_H = 0x8080808080808080ULL;
    Bitboard whitePawnAttacks = ((whitePawns & ~FILE_A) << 7) | ((whitePawns & ~FILE_H) << 9);
    Bitboard blackPawnAttacks = ((blackPawns & ~FILE_H) >> 7) | ((blackPawns & ~FILE_A) >> 9);

    Bitboard whiteKnights = board.pieces_[WHITE][KNIGHT];
    Bitboard whiteBishopBB = board.pieces_[WHITE][BISHOP];
    Bitboard whiteRooks = board.pieces_[WHITE][ROOK];
    Bitboard whiteQueens = board.pieces_[WHITE][QUEEN];
    Bitboard blackKnights = board.pieces_[BLACK][KNIGHT];
    Bitboard blackBishopBB = board.pieces_[BLACK][BISHOP];
    Bitboard blackRooks = board.pieces_[BLACK][ROOK];
    Bitboard blackQueens = board.pieces_[BLACK][QUEEN];

    int whitePawnCount = popCount(whitePawns);
    int blackPawnCount = popCount(blackPawns);
    int whiteKnightCount = popCount(whiteKnights);
    int blackKnightCount = popCount(blackKnights);
    int whiteBishopCount = popCount(whiteBishopBB);
    int blackBishopCount = popCount(blackBishopBB);
    int whiteRookCount = popCount(whiteRooks);
    int blackRookCount = popCount(blackRooks);
    int whiteQueenCount = popCount(whiteQueens);
    int blackQueenCount = popCount(blackQueens);

    int whiteKingSq = board.kingSq(WHITE);
    int blackKingSq = board.kingSq(BLACK);

    int whiteKingAttacksCount = 0, whiteKingAttacksWeight = 0;
    int blackKingAttacksCount = 0, blackKingAttacksWeight = 0;

    Bitboard whiteKingZone = pc.kingZoneMasks[whiteKingSq];
    Bitboard blackKingZone = pc.kingZoneMasks[blackKingSq];

    // White pawns
    Bitboard bb = whitePawns;
    while (bb) {
        int sq = popLSB(bb);
        int file = fileOf(sq);
        int rank = rankOf(sq);

        // Use tunable PST
        int pstValue = static_cast<int>(p.pst[PAWN][sq]);
        mgScore += static_cast<int>(p.pieceValues[PAWN]) + pstValue;
        egScore += static_cast<int>(p.pieceValues[PAWN]) + pstValue;

        if ((pc.fileMasks[sq] & whitePawns) != (1ULL << sq)) { mgScore -= 10; egScore -= 25; }

        if ((pc.neighborMasks[sq] & whitePawns) == 0) { mgScore -= 15; egScore -= 15; }
        else { mgScore += 10; egScore += 15; }

        if ((pc.whitePassedMasks[sq] & blackPawns) == 0) {
            int bonus = static_cast<int>(p.passedPawnBonus[rank]);
            int distOwn = std::abs(file - fileOf(whiteKingSq)) + std::abs(rank - rankOf(whiteKingSq));
            int distEnemy = std::abs(file - fileOf(blackKingSq)) + std::abs(rank - rankOf(blackKingSq));
            bonus += (distEnemy * 5) - (distOwn * 2);
            if (rank < 7 && (allPieces & (1ULL << (sq + 8)))) bonus /= 2;
            mgScore += bonus;
            egScore += bonus * 2;
            whitePassed = true;
        }
    }

    // Black pawns
    bb = blackPawns;
    while (bb) {
        int sq = popLSB(bb);
        int file = fileOf(sq);
        int rank = rankOf(sq);
        int pstSq = sq ^ 56;

        // Use tunable PST (mirrored for black)
        int pstValue = static_cast<int>(p.pst[PAWN][pstSq]);
        mgScore -= static_cast<int>(p.pieceValues[PAWN]) + pstValue;
        egScore -= static_cast<int>(p.pieceValues[PAWN]) + pstValue;

        if ((pc.fileMasks[sq] & blackPawns) != (1ULL << sq)) { mgScore += 10; egScore += 25; }

        if ((pc.neighborMasks[sq] & blackPawns) == 0) { mgScore += 15; egScore += 15; }
        else { mgScore -= 10; egScore -= 15; }

        if ((pc.blackPassedMasks[sq] & whitePawns) == 0) {
            int bonus = static_cast<int>(p.passedPawnBonus[7 - rank]);
            int distOwn = std::abs(file - fileOf(blackKingSq)) + std::abs(rank - rankOf(blackKingSq));
            int distEnemy = std::abs(file - fileOf(whiteKingSq)) + std::abs(rank - rankOf(whiteKingSq));
            bonus += (distEnemy * 5) - (distOwn * 2);
            if (rank > 0 && (allPieces & (1ULL << (sq - 8)))) bonus /= 2;
            mgScore -= bonus;
            egScore -= bonus * 2;
            blackPassed = true;
        }
    }

    // Other pieces
    for (int color = 0; color < 2; ++color) {
        bool isWhite = color == 0;
        Bitboard friendlyPawns = isWhite ? whitePawns : blackPawns;
        Bitboard enemyPawns = isWhite ? blackPawns : whitePawns;
        Bitboard enemyPawnAttacks = isWhite ? blackPawnAttacks : whitePawnAttacks;
        int enemyKingSq = isWhite ? blackKingSq : whiteKingSq;

        for (int pt = KNIGHT; pt <= KING; ++pt) {
            Bitboard pcs = board.pieces_[color][pt];
            while (pcs) {
                int sq = popLSB(pcs);
                int pstSq = isWhite ? sq : sq ^ 56;
                
                // Use tunable PST
                int pstValue = (pt == KING) ? static_cast<int>(p.pst[5][pstSq]) : static_cast<int>(p.pst[pt][pstSq]);
                int baseVal = static_cast<int>(p.pieceValues[pt]) + pstValue;
                int sign = isWhite ? 1 : -1;
                mgScore += sign * baseVal;
                
                if (pt == KING) {
                    // Use tunable king endgame PST
                    egScore += sign * (static_cast<int>(p.pieceValues[pt]) + static_cast<int>(p.kingEndgame[pstSq]));
                } else {
                    egScore += sign * baseVal;
                }

                phase += PIECE_PHASE[pt];

                if (pt != KING) {
                    Bitboard attacks;
                    switch (pt) {
                        case KNIGHT: attacks = Tables::KNIGHT_ATTACKS[sq]; break;
                        case BISHOP: attacks = bishopAttacks(sq, allPieces); break;
                        case ROOK:   attacks = rookAttacks(sq, allPieces);   break;
                        case QUEEN:  attacks = queenAttacks(sq, allPieces);  break;
                        default: attacks = 0; break;
                    }
                    Bitboard safeSquares = ~enemyPawnAttacks;
                    int safeMoves = popCount(attacks & safeSquares);
                    int unsafeMoves = popCount(attacks & enemyPawnAttacks);
                    // Safe moves count fully, unsafe moves count at 25%
                    double effectiveMobility = safeMoves + unsafeMoves * 0.25;
                    int mobScore = static_cast<int>(sign * effectiveMobility * p.mobilityBonus[pt]);
                    mgScore += mobScore;
                    egScore += mobScore;

                    Bitboard zoneHits = isWhite ? (attacks & blackKingZone) : (attacks & whiteKingZone);
                    if (zoneHits) {
                        int hits = popCount(zoneHits);
                        if (isWhite) {
                            blackKingAttacksCount++;
                            blackKingAttacksWeight += static_cast<int>(p.pieceAttackValue[pt] * hits);
                        } else {
                            whiteKingAttacksCount++;
                            whiteKingAttacksWeight += static_cast<int>(p.pieceAttackValue[pt] * hits);
                        }
                    }
                }

                if (pt == KNIGHT) {
                    int r = rankOf(sq);
                    bool outpostRank = isWhite ? (r >= 3 && r <= 5) : (r >= 2 && r <= 4);
                    if (outpostRank) {
                        Bitboard supportive = Tables::PAWN_ATTACKS[isWhite ? BLACK : WHITE][sq] & friendlyPawns;
                        Bitboard enemy = Tables::PAWN_ATTACKS[isWhite ? WHITE : BLACK][sq] & enemyPawns;
                        if (supportive && enemy == 0) {
                            mgScore += static_cast<int>(sign * p.knightOutpostBonus[0]);
                            egScore += static_cast<int>(sign * p.knightOutpostBonus[1]);
                        }
                    }
                } else if (pt == ROOK) {
                    if ((pc.fileMasks[sq] & friendlyPawns) == 0) {
                        int bonus = static_cast<int>(p.rookFileBonus[0]);
                        if ((pc.fileMasks[sq] & enemyPawns) == 0) bonus += static_cast<int>(p.rookFileBonus[1]);
                        int fileScore = sign * bonus;
                        mgScore += fileScore;
                        egScore += fileScore;
                    }
                    int relativeRank = isWhite ? rankOf(sq) : (7 - rankOf(sq));
                    if (relativeRank == 6) { // 7th rank
                        mgScore += sign * static_cast<int>(p.rookSeventhBonus[0]);
                        egScore += sign * static_cast<int>(p.rookSeventhBonus[1]);
                        // Extra bonus if enemy king is on back rank
                        int enemyKingRelRank = isWhite ? rankOf(blackKingSq) : (7 - rankOf(whiteKingSq));
                        if (enemyKingRelRank == 7) {
                            mgScore += sign * 10;
                            egScore += sign * 15;
                        }
                    }
                } else if (pt != KING && phase > 8) {
                    int dist = std::abs(fileOf(sq) - fileOf(enemyKingSq)) + std::abs(rankOf(sq) - rankOf(enemyKingSq));
                    if (dist <= 3) mgScore += sign * 6;
                }

                if (pt == BISHOP) {
                    bool light = ((fileOf(sq) + rankOf(sq)) & 1) == 0;
                    if (isWhite) {
                        if (light) whiteLightBishop = true; else whiteDarkBishop = true;
                    } else {
                        if (light) blackLightBishop = true; else blackDarkBishop = true;
                    }
                    if (isWhite) whiteBishops++; else blackBishops++;
    
                    // Penalize when own pawns block the bishop
                    Bitboard sameColorSquares = light ? 0x55AA55AA55AA55AAULL : 0xAA55AA55AA55AA55ULL;
                    Bitboard centralSquares = 0x00003C3C3C3C0000ULL; // Central 4x4 region (most important)
                    int blockedCentralPawns = popCount(friendlyPawns & sameColorSquares & centralSquares);
                    int blockedOtherPawns = popCount(friendlyPawns & sameColorSquares & ~centralSquares);
                    int badBishopPenalty = blockedCentralPawns * 12 + blockedOtherPawns * 4;
                    mgScore -= sign * badBishopPenalty;
                    egScore -= sign * (badBishopPenalty / 1.5); // Slightly less impactful in endgame
}
            }
        }
    }

    int devScore = evaluateDevelopment(board, phase);
    mgScore += devScore;

    if (phase > 0) {
        mgScore += evaluateKingSafety(board, true, whitePawns, blackPawns, phase, whiteKingAttacksCount, whiteKingAttacksWeight);
        mgScore -= evaluateKingSafety(board, false, blackPawns, whitePawns, phase, blackKingAttacksCount, blackKingAttacksWeight);
    }

    if (phase > 12) {
        int wRank = rankOf(whiteKingSq);
        int bRank = rankOf(blackKingSq);
        if (wRank < 2) {
            if ((pc.fileMasks[whiteKingSq] & whitePawns) == 0) mgScore -= 25;
            else if ((Tables::PAWN_ATTACKS[BLACK][whiteKingSq] & whitePawns) == 0) mgScore -= 10;
            if ((pc.neighborMasks[whiteKingSq] & whitePawns) == 0) mgScore -= 10;
        }
        if (bRank > 5) {
            if ((pc.fileMasks[blackKingSq] & blackPawns) == 0) mgScore += 25;
            else if ((Tables::PAWN_ATTACKS[WHITE][blackKingSq] & blackPawns) == 0) mgScore += 10;
            if ((pc.neighborMasks[blackKingSq] & blackPawns) == 0) mgScore += 10;
        }
    }

    if (whiteBishops >= 2) { mgScore += 20; egScore += 40; }
    if (blackBishops >= 2) { mgScore -= 20; egScore -= 40; }

    int totalNonPawn = whiteKnightCount + whiteBishopCount + whiteRookCount + whiteQueenCount +
                       blackKnightCount + blackBishopCount + blackRookCount + blackQueenCount;
    int totalPawns = whitePawnCount + blackPawnCount;

    // Insufficient material / trivial draws
    if (totalPawns == 0) {
        if (totalNonPawn == 0) return 0;  // KK
        if (totalNonPawn == 1 && (whiteBishopCount + blackBishopCount + whiteKnightCount + blackKnightCount) == 1) return 0;  // K+B v K or K+N v K
        if (totalNonPawn == 2 && whiteBishopCount == 1 && blackBishopCount == 1) return 0;  // KB v KB
    }

    int finalPhase = std::min(phase, 24);
    int score = (mgScore * finalPhase + egScore * (24 - finalPhase)) / 24;

    // Opposite-colored bishop dampening in low-material endings
    bool whiteOnlyLight = whiteLightBishop && !whiteDarkBishop && whiteBishopCount > 0;
    bool whiteOnlyDark = whiteDarkBishop && !whiteLightBishop && whiteBishopCount > 0;
    bool blackOnlyLight = blackLightBishop && !blackDarkBishop && blackBishopCount > 0;
    bool blackOnlyDark = blackDarkBishop && !blackLightBishop && blackBishopCount > 0;
    bool ocb = (whiteOnlyLight && blackOnlyDark) || (whiteOnlyDark && blackOnlyLight);
    bool noMajorsNoKnights = (whiteRookCount + blackRookCount + whiteQueenCount + blackQueenCount + whiteKnightCount + blackKnightCount) == 0;
    if (ocb && noMajorsNoKnights) {
        int scale = 4;
        if (!whitePassed && !blackPassed && totalPawns <= 4) scale = 8;
        score /= scale;
    }

    int fiftyRemaining = std::max(0, 100 - board.halfmove_);
    if (fiftyRemaining < 20) {
        score = score * fiftyRemaining / 20;
    }

    if (phase < 6) {
        bool winningWhite = score > 0;
        int winningKing = winningWhite ? whiteKingSq : blackKingSq;
        int losingKing = winningWhite ? blackKingSq : whiteKingSq;
        int lkRank = rankOf(losingKing);
        int lkFile = fileOf(losingKing);
        int lkDistCenter = std::max(3 - lkFile, lkFile - 4) + std::max(3 - lkRank, lkRank - 4);
        int distKings = std::abs(lkFile - fileOf(winningKing)) + std::abs(lkRank - rankOf(winningKing));
        int mopUp = lkDistCenter * 10 + (14 - distKings) * 4;
        if (winningWhite) score += mopUp; else score -= mopUp;
    }

    // tempo bonuses reduce performance. do not add any sort of tempo bonus.
    return board.sideToMove_ == WHITE ? score : -score;
}
} // namespace chess
