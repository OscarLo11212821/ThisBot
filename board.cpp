namespace chess {
class Board {
public:
    // Bitboards
    Bitboard pieces_[2][6]{};
    Bitboard byColor_[2]{};
    Bitboard occupied_{};
    
    // OPTIMIZATION: Mailbox for O(1) piece/color lookup
    std::uint8_t mailbox_[64];  // piece type in low 3 bits, color in bit 3, empty = 0xFF
    
    Color sideToMove_ = WHITE;
    CastlingRights castling_ = ALL_CASTLING;
    std::int8_t epSquare_ = -1;
    std::uint8_t halfmove_ = 0;
    std::uint16_t fullmove_ = 1;
    
    // Cached king squares for fast check detection
    std::uint8_t kingSquare_[2];

    // Position history for repetition detection
    std::vector<std::uint64_t> history_;
    std::uint64_t hash_ = 0;  // Current position hash

    void rebuildMailboxAndHash() {
        std::memset(mailbox_, 0xFF, sizeof(mailbox_));
        byColor_[0] = byColor_[1] = occupied_ = 0;
        kingSquare_[0] = kingSquare_[1] = 255;

        for (int c = 0; c < 2; ++c) {
            for (int pt = 0; pt < 6; ++pt) {
                Bitboard pcs = pieces_[c][pt];
                while (pcs) {
                    int sq = popLSB(pcs);
                    putPiece(static_cast<Color>(c), static_cast<PieceType>(pt), sq);
                }
            }
        }
        hash_ = computeHash();
    }

    Board() {
        Tables::init();
        Zobrist::init();  // Initialize Zobrist keys
        clear();
    }

    void clear() {
        std::memset(pieces_, 0, sizeof(pieces_));
        std::memset(mailbox_, 0xFF, sizeof(mailbox_));
        byColor_[0] = byColor_[1] = occupied_ = 0;
        sideToMove_ = WHITE;
        castling_ = NO_CASTLING;
        epSquare_ = -1;
        halfmove_ = 0;
        fullmove_ = 1;
        kingSquare_[0] = kingSquare_[1] = 255;
        history_.clear();
        hash_ = 0;
    }

    // Compute hash from scratch
    std::uint64_t computeHash() const {
        std::uint64_t h = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if (mailbox_[sq] != 0xFF) {
                int pt = mailbox_[sq] & 7;
                int c = (mailbox_[sq] >> 3) & 1;
                h ^= Zobrist::PIECE[c][pt][sq];
            }
        }
        h ^= Zobrist::CASTLING[castling_];
        if (epSquare_ >= 0) h ^= Zobrist::EP[epSquare_ & 7];
        if (sideToMove_ == BLACK) h ^= Zobrist::SIDE;
        return h;
    }

    // Check for repetition (returns true if position occurred before)
    // For 3-fold, we need the position to appear 2 more times (already appeared once = current)
    bool isRepetition(int searchPly = 0) const {
        if (history_.size() < 4) return false;  // Need at least 4 plies for repetition
        
        int count = 0;
        // Only check positions since last irreversible move (limited by halfmove clock)
        // Also limit by searchPly to only count positions from before the search started
        int limit = std::min(static_cast<int>(history_.size()), static_cast<int>(halfmove_));
        
        for (int i = 2; i <= limit; i += 2) {  // Step by 2 (same side to move)
            if (history_[history_.size() - i] == hash_) {
                count++;
                if (count >= 2) return true;  // 3-fold (current + 2 previous)
                // In search, we can return true on first repetition for efficiency
                if (searchPly > 0 && count >= 1) return true;
            }
        }
        return false;
    }

    // OPTIMIZATION: Inline piece manipulation
    FORCE_INLINE void putPiece(Color c, PieceType pt, int sq) {
        Bitboard bb = squareBB(sq);
        pieces_[c][pt] |= bb;
        byColor_[c] |= bb;
        occupied_ |= bb;
        mailbox_[sq] = pt | (c << 3);
        if (pt == KING) kingSquare_[c] = sq;
    }
    
    FORCE_INLINE void removePiece(Color c, PieceType pt, int sq) {
        Bitboard bb = squareBB(sq);
        pieces_[c][pt] &= ~bb;
        byColor_[c] &= ~bb;
        occupied_ &= ~bb;
        mailbox_[sq] = 0xFF;
    }
    
    FORCE_INLINE void movePiece(Color c, PieceType pt, int from, int to) {
        Bitboard fromTo = squareBB(from) | squareBB(to);
        pieces_[c][pt] ^= fromTo;
        byColor_[c] ^= fromTo;
        occupied_ ^= fromTo;
        mailbox_[from] = 0xFF;
        mailbox_[to] = pt | (c << 3);
        if (pt == KING) kingSquare_[c] = to;
    }
    
    // OPTIMIZATION: O(1) piece type lookup
    FORCE_INLINE PieceType pieceAt(int sq) const {
        return PieceType(mailbox_[sq] & 7);
    }
    
    FORCE_INLINE Color colorAt(int sq) const {
        return Color((mailbox_[sq] >> 3) & 1);
    }
    
    FORCE_INLINE bool isEmpty(int sq) const {
        return mailbox_[sq] == 0xFF;
    }
    
    FORCE_INLINE int kingSq(Color c) const {
        return kingSquare_[c];
    }

    //========================================================================
    // Attack detection
    //========================================================================
    FORCE_INLINE Bitboard attackersTo(int sq, Bitboard occ) const {
        return (Tables::PAWN_ATTACKS[BLACK][sq] & pieces_[WHITE][PAWN])
             | (Tables::PAWN_ATTACKS[WHITE][sq] & pieces_[BLACK][PAWN])
             | (Tables::KNIGHT_ATTACKS[sq] & (pieces_[WHITE][KNIGHT] | pieces_[BLACK][KNIGHT]))
             | (Tables::KING_ATTACKS[sq] & (pieces_[WHITE][KING] | pieces_[BLACK][KING]))
             | (rookAttacks(sq, occ) & (pieces_[WHITE][ROOK] | pieces_[WHITE][QUEEN] | 
                                        pieces_[BLACK][ROOK] | pieces_[BLACK][QUEEN]))
             | (bishopAttacks(sq, occ) & (pieces_[WHITE][BISHOP] | pieces_[WHITE][QUEEN] |
                                          pieces_[BLACK][BISHOP] | pieces_[BLACK][QUEEN]));
    }
    
    FORCE_INLINE Bitboard attackersTo(int sq, Bitboard occ, Color attacker) const {
        return (Tables::PAWN_ATTACKS[~attacker][sq] & pieces_[attacker][PAWN])
             | (Tables::KNIGHT_ATTACKS[sq] & pieces_[attacker][KNIGHT])
             | (Tables::KING_ATTACKS[sq] & pieces_[attacker][KING])
             | (rookAttacks(sq, occ) & (pieces_[attacker][ROOK] | pieces_[attacker][QUEEN]))
             | (bishopAttacks(sq, occ) & (pieces_[attacker][BISHOP] | pieces_[attacker][QUEEN]));
    }
    
    FORCE_INLINE bool isAttacked(int sq, Color attacker) const {
        // OPTIMIZATION: Check cheap attacks first (knights, pawns)
        if (Tables::KNIGHT_ATTACKS[sq] & pieces_[attacker][KNIGHT]) return true;
        if (Tables::PAWN_ATTACKS[~attacker][sq] & pieces_[attacker][PAWN]) return true;
        if (Tables::KING_ATTACKS[sq] & pieces_[attacker][KING]) return true;
        
        Bitboard rq = pieces_[attacker][ROOK] | pieces_[attacker][QUEEN];
        if (rq && (rookAttacks(sq, occupied_) & rq)) return true;
        
        Bitboard bq = pieces_[attacker][BISHOP] | pieces_[attacker][QUEEN];
        if (bq && (bishopAttacks(sq, occupied_) & bq)) return true;
        
        return false;
    }
    
    FORCE_INLINE Bitboard checkers() const {
        return attackersTo(kingSq(sideToMove_), occupied_, ~sideToMove_);
    }
    
    FORCE_INLINE bool inCheck() const {
        return isAttacked(kingSq(sideToMove_), ~sideToMove_);
    }

    //========================================================================
    // OPTIMIZATION 11: Undo info - compact
    //========================================================================
    struct UndoInfo {
        std::uint32_t data;  // packed: castling, ep, halfmove, captured piece
        
        FORCE_INLINE CastlingRights castling() const { return CastlingRights(data & 0xF); }
        FORCE_INLINE int ep() const { return ((data >> 4) & 0x7F) - 1; }
        FORCE_INLINE int halfmove() const { return (data >> 11) & 0xFF; }
        FORCE_INLINE PieceType captured() const { return PieceType((data >> 19) & 0x7); }
        FORCE_INLINE bool isInvalid() const { return (data & 0x80000000u) != 0; }
        
        static UndoInfo make(CastlingRights c, int ep, int hm, PieceType cap) {
            return {std::uint32_t(c) | (std::uint32_t(ep + 1) << 4) | 
                   (std::uint32_t(hm) << 11) | (std::uint32_t(cap) << 19)};
        }
        static UndoInfo makeInvalid() { return {0x80000000u}; }
    };

    struct NullUndo {
        std::int8_t ep;
        std::uint8_t halfmove;
        CastlingRights castling;
    };

    //========================================================================
    // Make/Unmake moves - optimized with incremental hash updates
    //========================================================================
    // Update makeMove to track hash incrementally and update history
    UndoInfo makeMove(Move m) {
        // Save current hash to history BEFORE making the move
        history_.push_back(hash_);
        
        Color us = sideToMove_;
        Color them = ~us;
        int from = m.from(), to = m.to();
        MoveType mt = m.type();
        PieceType moving = pieceAt(from);
        if (moving == NO_PIECE) {
            // Repair any mailbox/piece bitboard divergence and retry
            rebuildMailboxAndHash();
            if (!history_.empty()) history_.back() = hash_;
            moving = pieceAt(from);
        }
        if (moving == NO_PIECE) {
            // Bail out gracefully if the move is clearly invalid
            return UndoInfo::makeInvalid();
        }
        PieceType captured = isEmpty(to) ? NO_PIECE : pieceAt(to);
        
        UndoInfo undo = UndoInfo::make(castling_, epSquare_, halfmove_, captured);
        
        // Update hash: remove old castling rights and ep
        hash_ ^= Zobrist::CASTLING[castling_];
        if (epSquare_ >= 0) hash_ ^= Zobrist::EP[epSquare_ & 7];
        
        halfmove_ = (moving == PAWN || captured != NO_PIECE) ? 0 : halfmove_ + 1;
        epSquare_ = -1;
        
        if (mt == MT_CASTLING) {
            int side = (fileOf(to) == 6) ? 0 : 1;
            int idx = us * 2 + side;
            
            // Update hash for king move
            hash_ ^= Zobrist::PIECE[us][KING][from];
            hash_ ^= Zobrist::PIECE[us][KING][to];
            // Update hash for rook move
            hash_ ^= Zobrist::PIECE[us][ROOK][Tables::CASTLING_ROOK_FROM[idx]];
            hash_ ^= Zobrist::PIECE[us][ROOK][Tables::CASTLING_ROOK_TO[idx]];
            
            movePiece(us, KING, from, to);
            movePiece(us, ROOK, Tables::CASTLING_ROOK_FROM[idx], Tables::CASTLING_ROOK_TO[idx]);
        } else {
            if (captured != NO_PIECE) {
                hash_ ^= Zobrist::PIECE[them][captured][to];
                removePiece(them, captured, to);
            }
            
            if (mt == MT_EN_PASSANT) {
                int capSq = to + (us == WHITE ? -8 : 8);
                hash_ ^= Zobrist::PIECE[them][PAWN][capSq];
                removePiece(them, PAWN, capSq);
            }
            
            if (mt == MT_PROMOTION) {
                hash_ ^= Zobrist::PIECE[us][PAWN][from];
                hash_ ^= Zobrist::PIECE[us][m.promo()][to];
                removePiece(us, PAWN, from);
                putPiece(us, m.promo(), to);
            } else {
                hash_ ^= Zobrist::PIECE[us][moving][from];
                hash_ ^= Zobrist::PIECE[us][moving][to];
                movePiece(us, moving, from, to);
                
                if (moving == PAWN && std::abs(to - from) == 16) {
                    epSquare_ = (from + to) / 2;
                }
            }
        }
        
        // Update castling rights
        castling_ &= Tables::CASTLING_RIGHTS_MASK[from];
        castling_ &= Tables::CASTLING_RIGHTS_MASK[to];
        
        // Add new castling rights and ep to hash
        hash_ ^= Zobrist::CASTLING[castling_];
        if (epSquare_ >= 0) hash_ ^= Zobrist::EP[epSquare_ & 7];
        
        // Flip side
        hash_ ^= Zobrist::SIDE;
        sideToMove_ = them;
        fullmove_ += (us == BLACK);
        
        return undo;
    }
    
    void unmakeMove(Move m, UndoInfo undo) {
        if (undo.isInvalid()) {
            if (!history_.empty()) history_.pop_back();
            return;
        }
        // Restore hash from history
        hash_ = history_.back();
        history_.pop_back();
        
        sideToMove_ = ~sideToMove_;
        Color us = sideToMove_;
        Color them = ~us;
        int from = m.from(), to = m.to();
        MoveType mt = m.type();
        
        fullmove_ -= (us == BLACK);
        castling_ = undo.castling();
        epSquare_ = undo.ep();
        halfmove_ = undo.halfmove();
        
        if (mt == MT_CASTLING) {
            int side = (fileOf(to) == 6) ? 0 : 1;
            int idx = us * 2 + side;
            movePiece(us, KING, to, from);
            movePiece(us, ROOK, Tables::CASTLING_ROOK_TO[idx], Tables::CASTLING_ROOK_FROM[idx]);
        } else {
            if (mt == MT_PROMOTION) {
                removePiece(us, m.promo(), to);
                putPiece(us, PAWN, from);
            } else {
                movePiece(us, pieceAt(to), to, from);
            }
            
            if (mt == MT_EN_PASSANT) {
                int capSq = to + (us == WHITE ? -8 : 8);
                putPiece(them, PAWN, capSq);
            } else if (undo.captured() != NO_PIECE) {
                putPiece(them, undo.captured(), to);
            }
        }
    }

    // Null move (for search pruning)
    NullUndo makeNullMove() {
        NullUndo u{epSquare_, halfmove_, castling_};

        // Update hash for null move
        if (epSquare_ >= 0) hash_ ^= Zobrist::EP[epSquare_ & 7];
        hash_ ^= Zobrist::SIDE;

        epSquare_ = -1;
        halfmove_ = 0;
        sideToMove_ = ~sideToMove_;
        return u;
    }

    void unmakeNullMove(NullUndo u) {
        // Restore hash
        hash_ ^= Zobrist::SIDE;
        if (u.ep >= 0) hash_ ^= Zobrist::EP[u.ep & 7];

        sideToMove_ = ~sideToMove_;
        epSquare_ = u.ep;
        halfmove_ = u.halfmove;
        castling_ = u.castling;
    }

    //========================================================================
    // OPTIMIZATION 12: Fast legal move generation with pin detection
    //========================================================================
    void generateLegalMoves(MoveList& moves) {
        moves.clear();
        
        Color us = sideToMove_;
        Color them = ~us;
        int ksq = kingSq(us);
        Bitboard checkersBB = checkers();
        int numCheckers = popCount(checkersBB);
        
        // OPTIMIZATION: Precompute pins
        Bitboard pinned = 0;
        
        // FIX: Mask out our own pieces to see "through" them for pinners
        Bitboard occNoUs = occupied_ ^ byColor_[us];
        
        Bitboard pinners = ((rookAttacks(ksq, occNoUs) & (pieces_[them][ROOK] | pieces_[them][QUEEN]))
                          | (bishopAttacks(ksq, occNoUs) & (pieces_[them][BISHOP] | pieces_[them][QUEEN])));
        
        while (pinners) {
            int pinner = popLSB(pinners);
            Bitboard between = Tables::BETWEEN_BB[ksq][pinner] & occupied_;
            // A piece is pinned if it is the ONLY thing between King and Pinner
            if (popCount(between) == 1) {
                pinned |= between & byColor_[us];
            }
        }
        
        // King moves - always generated
        Bitboard kingMoves = Tables::KING_ATTACKS[ksq] & ~byColor_[us];
        while (kingMoves) {
            int to = popLSB(kingMoves);
            // Must check if destination is attacked (king moved away)
            Bitboard newOcc = (occupied_ ^ squareBB(ksq)) | squareBB(to);
            if (!attackersTo(to, newOcc, them)) {
                moves.push(Move(ksq, to));
            }
        }
        
        // If double check, only king moves are legal
        if (numCheckers > 1) return;
        
        Bitboard targetMask = ~0ULL;  // All squares
        if (numCheckers == 1) {
            // Must block or capture the checker
            int checkerSq = lsb(checkersBB);
            targetMask = Tables::BETWEEN_BB[ksq][checkerSq] | checkersBB;
        }
        
        // Generate non-king moves
        generatePieceMoves<PAWN>(moves, us, them, ksq, pinned, targetMask);
        generatePieceMoves<KNIGHT>(moves, us, them, ksq, pinned, targetMask);
        generatePieceMoves<BISHOP>(moves, us, them, ksq, pinned, targetMask);
        generatePieceMoves<ROOK>(moves, us, them, ksq, pinned, targetMask);
        generatePieceMoves<QUEEN>(moves, us, them, ksq, pinned, targetMask);
        
        // Castling - only if not in check
        if (numCheckers == 0) {
            generateCastling(moves, us, them);
        }
        
        // En passant
        if (epSquare_ >= 0) {
            generateEnPassant(moves, us, them);
        }
    }
    
private:
    template<PieceType PT>
    void generatePieceMoves(MoveList& moves, Color us, Color them, int ksq, 
                            Bitboard pinned, Bitboard targetMask) {
        Bitboard pcs = pieces_[us][PT];
        
        while (pcs) {
            int from = popLSB(pcs);
            bool isPinned = pinned & squareBB(from);
            
            Bitboard attacks;
            if constexpr (PT == PAWN) {
                generatePawnMoves(moves, from, us, them, ksq, isPinned, targetMask);
                continue;
            } else if constexpr (PT == KNIGHT) {
                if (isPinned) continue;  // Pinned knight can never move
                attacks = Tables::KNIGHT_ATTACKS[from];
            } else if constexpr (PT == BISHOP) {
                attacks = bishopAttacks(from, occupied_);
            } else if constexpr (PT == ROOK) {
                attacks = rookAttacks(from, occupied_);
            } else if constexpr (PT == QUEEN) {
                attacks = queenAttacks(from, occupied_);
            }
            
            attacks &= ~byColor_[us] & targetMask;
            
            // If pinned, can only move along pin ray
            if (isPinned) {
                attacks &= Tables::LINE_BB[ksq][from];
            }
            
            while (attacks) {
                moves.push(Move(from, popLSB(attacks)));
            }
        }
    }
    
    void generatePawnMoves(MoveList& moves, int from, Color us, Color them, int ksq,
                           bool isPinned, Bitboard targetMask) {
        int forward = us == WHITE ? 8 : -8;
        int startRank = us == WHITE ? 1 : 6;
        int promoRank = us == WHITE ? 6 : 1;
        int rank = rankOf(from);
        
        Bitboard pinRay = isPinned ? Tables::LINE_BB[ksq][from] : ~0ULL;
        
        // Forward push
        int to = from + forward;
        
        // General requirements for any push: Dest empty, and must stay on pin ray (vertical)
        if (isEmpty(to) && (squareBB(to) & pinRay)) {
            
            // Single push validity: Must satisfy targetMask (block check)
            if (squareBB(to) & targetMask) {
                if (rank == promoRank) {
                    moves.push(Move(from, to, MT_PROMOTION, QUEEN));
                    moves.push(Move(from, to, MT_PROMOTION, ROOK));
                    moves.push(Move(from, to, MT_PROMOTION, BISHOP));
                    moves.push(Move(from, to, MT_PROMOTION, KNIGHT));
                } else {
                    moves.push(Move(from, to));
                }
            }
            
            // Double push validity
            // FIX: Decoupled from single push targetMask check. 
            // The intermediate square `to` does NOT need to block the check, only `to2` does.
            if (rank == startRank) {
                int to2 = to + forward;
                if (isEmpty(to2) && (squareBB(to2) & targetMask) && (squareBB(to2) & pinRay)) {
                    moves.push(Move(from, to2));
                }
            }
        }
        
        // Captures
        Bitboard attacks = Tables::PAWN_ATTACKS[us][from] & byColor_[them] & targetMask & pinRay;
        while (attacks) {
            int capSq = popLSB(attacks);
            if (rank == promoRank) {
                moves.push(Move(from, capSq, MT_PROMOTION, QUEEN));
                moves.push(Move(from, capSq, MT_PROMOTION, ROOK));
                moves.push(Move(from, capSq, MT_PROMOTION, BISHOP));
                moves.push(Move(from, capSq, MT_PROMOTION, KNIGHT));
            } else {
                moves.push(Move(from, capSq));
            }
        }
    }
    
    void generateEnPassant(MoveList& moves, Color us, Color them) {
        if (epSquare_ < 0) return;
    
        // Pawns that could capture en passant
        Bitboard pawns = pieces_[us][PAWN] & Tables::PAWN_ATTACKS[them][epSquare_];
    
        while (pawns) {
            int from = popLSB(pawns);
    
            Move m(from, epSquare_, MT_EN_PASSANT);
            UndoInfo u = makeMove(m);
    
            // After makeMove, sideToMove_ == them.
            // We want to know if *our* king (us) is in check.
            bool legal = !isAttacked(kingSq(us), them);
    
            unmakeMove(m, u);
    
            if (legal)
                moves.push(m);
        }
    }
    
    void generateCastling(MoveList& moves, Color us, Color them) {
        int ksq = us == WHITE ? 4 : 60;
        
        // Kingside
        CastlingRights oo = us == WHITE ? WHITE_OO : BLACK_OO;
        if (castling_ & oo) {
            int f1 = ksq + 1, g1 = ksq + 2;
            if (isEmpty(f1) && isEmpty(g1) &&
                !isAttacked(f1, them) && !isAttacked(g1, them)) {
                moves.push(Move(ksq, g1, MT_CASTLING));
            }
        }
        
        // Queenside
        CastlingRights ooo = us == WHITE ? WHITE_OOO : BLACK_OOO;
        if (castling_ & ooo) {
            int d1 = ksq - 1, c1 = ksq - 2, b1 = ksq - 3;
            if (isEmpty(d1) && isEmpty(c1) && isEmpty(b1) &&
                !isAttacked(d1, them) && !isAttacked(c1, them)) {
                moves.push(Move(ksq, c1, MT_CASTLING));
            }
        }
    }

public:
    //========================================================================
    // FEN parsing/output
    //========================================================================
    bool setFEN(const std::string& fen) {
        clear();
        std::istringstream ss(fen);
        std::string pieces, side, castling, ep;
        int hm = 0, fm = 1;
        ss >> pieces >> side >> castling >> ep >> hm >> fm;
        
        int sq = 56;
        for (char c : pieces) {
            if (c == '/') sq -= 16;
            else if (c >= '1' && c <= '8') sq += c - '0';
            else {
                Color col = std::isupper(c) ? WHITE : BLACK;
                PieceType pt = charToPT(c);
                if (pt != NO_PIECE && sq >= 0 && sq < 64) putPiece(col, pt, sq++);
            }
        }
        
        sideToMove_ = (side == "b") ? BLACK : WHITE;
        
        castling_ = NO_CASTLING;
        for (char c : castling) {
            if (c == 'K') castling_ = castling_ | WHITE_OO;
            if (c == 'Q') castling_ = castling_ | WHITE_OOO;
            if (c == 'k') castling_ = castling_ | BLACK_OO;
            if (c == 'q') castling_ = castling_ | BLACK_OOO;
        }
        
        epSquare_ = strToSq(ep);
        halfmove_ = hm;
        fullmove_ = fm;
        
        // Initialize hash after setting up the position
        hash_ = computeHash();
        history_.clear();  // Clear history when setting new position
        
        return true;
    }
    
    std::string toFEN() const {
        std::string fen;
        for (int r = 7; r >= 0; --r) {
            int empty = 0;
            for (int f = 0; f < 8; ++f) {
                int sq = r * 8 + f;
                if (isEmpty(sq)) { ++empty; continue; }
                if (empty) { fen += char('0' + empty); empty = 0; }
                fen += ptToChar(pieceAt(sq), colorAt(sq));
            }
            if (empty) fen += char('0' + empty);
            if (r > 0) fen += '/';
        }
        fen += sideToMove_ == WHITE ? " w " : " b ";
        if (castling_ == NO_CASTLING) fen += '-';
        else {
            if (castling_ & WHITE_OO) fen += 'K';
            if (castling_ & WHITE_OOO) fen += 'Q';
            if (castling_ & BLACK_OO) fen += 'k';
            if (castling_ & BLACK_OOO) fen += 'q';
        }
        fen += ' ';
        fen += epSquare_ >= 0 ? sqStr(epSquare_) : "-";
        fen += ' ' + std::to_string(halfmove_) + ' ' + std::to_string(fullmove_);
        return fen;
    }
    
    void reset() { setFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
    
    //========================================================================
    // Move parsing
    //========================================================================
    std::optional<Move> parseUCI(const std::string& s) {
        if (s.length() < 4) return std::nullopt;
        int from = strToSq(s.substr(0, 2));
        int to = strToSq(s.substr(2, 2));
        if (from < 0 || to < 0) return std::nullopt;
        
        MoveList moves;
        generateLegalMoves(moves);
        
        PieceType promo = s.length() >= 5 ? charToPT(s[4]) : NO_PIECE;
        
        for (Move m : moves) {
            if (m.from() == from && m.to() == to) {
                if (m.type() == MT_PROMOTION) {
                    if (m.promo() == promo) return m;
                } else {
                    return m;
                }
            }
        }
        return std::nullopt;
    }
    
    std::string moveToUCI(Move m) const {
        std::string s = sqStr(m.from()) + sqStr(m.to());
        if (m.type() == MT_PROMOTION) s += std::tolower(ptToChar(m.promo(), WHITE));
        return s;
    }
    
    //========================================================================
    // Game state
    //========================================================================
    bool isCheckmate() { MoveList m; generateLegalMoves(m); return m.size() == 0 && inCheck(); }
    bool isStalemate() { MoveList m; generateLegalMoves(m); return m.size() == 0 && !inCheck(); }
    bool isDraw(int searchPly = 0) {
        return halfmove_ >= 100 || isRepetition(searchPly);
    }
    
    //========================================================================
    // Display
    //========================================================================
    void print() const {
        std::cout << "\n";
        for (int r = 7; r >= 0; --r) {
            std::cout << (r + 1) << " |";
            for (int f = 0; f < 8; ++f) {
                int sq = r * 8 + f;
                std::cout << ' ' << (isEmpty(sq) ? '.' : ptToChar(pieceAt(sq), colorAt(sq)));
            }
            std::cout << '\n';
        }
        std::cout << "   ----------------\n    a b c d e f g h\n\n";
        std::cout << "FEN: " << toFEN() << "\n";
        std::cout << (sideToMove_ == WHITE ? "White" : "Black") << " to move";
        if (inCheck()) std::cout << " (CHECK)";
        std::cout << "\n";
    }
};

//======================================================================
// Evaluation parameters
//======================================================================
} // namespace chess
