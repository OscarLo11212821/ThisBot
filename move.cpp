namespace chess {
enum MoveType : std::uint8_t {
    MT_NORMAL = 0, MT_PROMOTION = 1, MT_EN_PASSANT = 2, MT_CASTLING = 3
};

class Move {
    std::uint32_t data_;
public:
    constexpr Move() : data_(0) {}
    constexpr Move(int from, int to, MoveType mt = MT_NORMAL, PieceType promo = KNIGHT)
        : data_(from | (to << 6) | (mt << 12) | ((promo - KNIGHT) << 14)) {}
    
    FORCE_INLINE int from() const { return data_ & 0x3F; }
    FORCE_INLINE int to() const { return (data_ >> 6) & 0x3F; }
    FORCE_INLINE MoveType type() const { return MoveType((data_ >> 12) & 0x3); }
    FORCE_INLINE PieceType promo() const { return PieceType(((data_ >> 14) & 0x3) + KNIGHT); }
    FORCE_INLINE bool isNull() const { return data_ == 0; }
    FORCE_INLINE bool operator==(Move other) const { return data_ == other.data_; }
    FORCE_INLINE std::uint32_t raw() const { return data_; }
};

//============================================================================
// OPTIMIZATION 6: Fixed-size move list (no heap allocation)
// Max possible moves in any position is ~218
//============================================================================
class MoveList {
    std::array<Move, 256> moves_;
    int size_ = 0;
public:
    FORCE_INLINE void push(Move m) { moves_[size_++] = m; }
    FORCE_INLINE void clear() { size_ = 0; }
    FORCE_INLINE int size() const { return size_; }
    FORCE_INLINE Move operator[](int i) const { return moves_[i]; }
    FORCE_INLINE Move& operator[](int i) { return moves_[i]; }  // Non-const for sorting
    FORCE_INLINE Move* begin() { return moves_.data(); }
    FORCE_INLINE Move* end() { return moves_.data() + size_; }
    FORCE_INLINE const Move* begin() const { return moves_.data(); }
    FORCE_INLINE const Move* end() const { return moves_.data() + size_; }
};

//============================================================================
// OPTIMIZATION 7: Precomputed attack tables with better layout
//============================================================================

// String utilities
//============================================================================
inline std::string sqStr(int sq) {
    return sq >= 0 && sq < 64 
        ? std::string{char('a' + fileOf(sq)), char('1' + rankOf(sq))}
        : "-";
}

inline int strToSq(const std::string& s) {
    if (s.length() < 2 || s == "-") return -1;
    int f = s[0] - 'a', r = s[1] - '1';
    return (f >= 0 && f < 8 && r >= 0 && r < 8) ? r * 8 + f : -1;
}

inline PieceType charToPT(char c) {
    switch (std::toupper(c)) {
        case 'P': return PAWN; case 'N': return KNIGHT; case 'B': return BISHOP;
        case 'R': return ROOK; case 'Q': return QUEEN; case 'K': return KING;
        default: return NO_PIECE;
    }
}

inline char ptToChar(PieceType pt, Color c) {
    static constexpr char ch[] = "PNBRQK?";
    char r = ch[pt];
    return c == BLACK ? std::tolower(r) : r;
}

//============================================================================
// OPTIMIZATION 10: Board with mailbox for O(1) piece lookup
//============================================================================
} // namespace chess
