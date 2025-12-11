namespace chess {

using Bitboard = std::uint64_t;

//============================================================================
// OPTIMIZATION 1: Force inline for hot paths
//============================================================================
#if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline __attribute__((always_inline))
#else
    #define FORCE_INLINE inline
#endif

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

//============================================================================
// OPTIMIZATION 2: Faster bit operations with intrinsics
//============================================================================
FORCE_INLINE int lsb(Bitboard bb) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(bb);
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return static_cast<int>(idx);
#else
    // DeBruijn fallback
    static constexpr int DeBruijnTable[64] = {
        0,47,1,56,48,27,2,60,57,49,41,37,28,16,3,61,
        54,58,35,52,50,42,21,44,38,32,29,23,17,11,4,62,
        46,55,26,59,40,36,15,53,34,51,20,43,31,22,10,45,
        25,39,14,33,19,30,9,24,13,18,8,12,7,6,5,63
    };
    return DeBruijnTable[((bb ^ (bb - 1)) * 0x03f79d71b4cb0a89ULL) >> 58];
#endif
}

FORCE_INLINE int popLSB(Bitboard& bb) {
    int sq = lsb(bb);
    bb &= bb - 1;  // Clear LSB - branchless
    return sq;
}

FORCE_INLINE int popCount(Bitboard bb) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(bb);
#elif defined(_MSC_VER)
    return static_cast<int>(__popcnt64(bb));
#else
    // SWAR popcount
    bb -= (bb >> 1) & 0x5555555555555555ULL;
    bb = (bb & 0x3333333333333333ULL) + ((bb >> 2) & 0x3333333333333333ULL);
    bb = (bb + (bb >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return static_cast<int>((bb * 0x0101010101010101ULL) >> 56);
#endif
}

FORCE_INLINE int rankOf(int sq) { return sq >> 3; }
FORCE_INLINE int fileOf(int sq) { return sq & 7; }
FORCE_INLINE constexpr Bitboard squareBB(int sq) { return 1ULL << sq; }

//============================================================================
// OPTIMIZATION 3: Compile-time constants
//============================================================================
constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_H = FILE_A << 7;
constexpr Bitboard RANK_1 = 0xFFULL;
constexpr Bitboard RANK_2 = RANK_1 << 8;
constexpr Bitboard RANK_7 = RANK_1 << 48;
constexpr Bitboard RANK_8 = RANK_1 << 56;
constexpr Bitboard NOT_FILE_A = ~FILE_A;
constexpr Bitboard NOT_FILE_H = ~(FILE_A << 7);

//============================================================================
// OPTIMIZATION 4: Compact enums (uint8_t where possible)
//============================================================================
enum Color : std::uint8_t { WHITE = 0, BLACK = 1 };
enum PieceType : std::uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE = 7 };

FORCE_INLINE constexpr Color operator~(Color c) { return Color(c ^ 1); }

enum CastlingRights : std::uint8_t {
    NO_CASTLING = 0, WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8,
    WHITE_CASTLE = 3, BLACK_CASTLE = 12, ALL_CASTLING = 15
};

FORCE_INLINE CastlingRights operator|(CastlingRights a, CastlingRights b) { return CastlingRights(a | int(b)); }
FORCE_INLINE CastlingRights operator&(CastlingRights a, CastlingRights b) { return CastlingRights(a & int(b)); }
FORCE_INLINE CastlingRights& operator&=(CastlingRights& a, CastlingRights b) { return a = a & b; }

//============================================================================
// OPTIMIZATION 5: Compact Move (32-bit instead of struct with padding)
// Encoding: [from:6][to:6][flags:4][promo:3][captured:3] + padding
//============================================================================

namespace Tables {

alignas(64) Bitboard KING_ATTACKS[64];
alignas(64) Bitboard KNIGHT_ATTACKS[64];
alignas(64) Bitboard PAWN_ATTACKS[2][64];
alignas(64) Bitboard BETWEEN_BB[64][64];
alignas(64) Bitboard LINE_BB[64][64];

// Magic bitboard tables
alignas(64) Bitboard ROOK_TABLE[0x19000];
alignas(64) Bitboard BISHOP_TABLE[0x1480];

struct MagicEntry {
    Bitboard mask;
    Bitboard magic;
    Bitboard* attacks;
    int shift;
};

alignas(64) MagicEntry ROOK_MAGICS[64];
alignas(64) MagicEntry BISHOP_MAGICS[64];

constexpr Bitboard ROOK_MAGIC_NUMBERS[64] = {
    0x80001020804000ULL, 0x640001008200040ULL, 0x680100080200088ULL, 0x480080080b0004cULL,
    0x42800a1400800800ULL, 0x100010002040008ULL, 0x8080020001000080ULL, 0x8100014c29000482ULL,
    0x4800480400020ULL, 0x2040401000200048ULL, 0x2411001041002000ULL, 0x800801000800804ULL,
    0x2004808004000800ULL, 0x600800400800200ULL, 0x200a200090c48ULL, 0x32000102004084ULL,
    0x440808000304004ULL, 0x810024020004000ULL, 0x4010002000280400ULL, 0x4000808010000802ULL,
    0x108818004001800ULL, 0x20808004000200ULL, 0x4120010100040200ULL, 0x2201a000080c304ULL,
    0x8810810200220040ULL, 0x4800400180200080ULL, 0x180802200420010ULL, 0x100080080080ULL,
    0xa01028500080010ULL, 0x84000202000810ULL, 0x2006004200014428ULL, 0x2142030200008474ULL,
    0x6400804003800120ULL, 0x1a0400080802000ULL, 0xa000841004802001ULL, 0x1002009001002ULL,
    0x40080800800ULL, 0x3000209000400ULL, 0x10028104001008ULL, 0x5100005896000504ULL,
    0x800308040008000ULL, 0x4020002040008080ULL, 0x41002000410010ULL, 0x30001008008080ULL,
    0x4000040008008080ULL, 0x1000204010008ULL, 0x800810022c0019ULL, 0x1000080410002ULL,
    0x80002000400040ULL, 0x500804002201280ULL, 0x212430020001100ULL, 0x4200841120200ULL,
    0x3001008040201002ULL, 0x2204000200410040ULL, 0x808090210082c00ULL, 0x48c800100004080ULL,
    0x160410020108001ULL, 0xa0400024110481ULL, 0x108429102a001ULL, 0x480090004201001ULL,
    0x2000410200802ULL, 0x200100080400862dULL, 0x80c10004020010a1ULL, 0x204812402508502ULL
};

constexpr Bitboard BISHOP_MAGIC_NUMBERS[64] = {
    0x240018105020881ULL, 0x80410a2820a8408ULL, 0x11000a200400000ULL, 0x832411d202014c80ULL,
    0x20202108100480aULL, 0x1102230080008ULL, 0x40d6010960b01081ULL, 0x203808288200204ULL,
    0x200a1121a0e0400ULL, 0x820002100c030548ULL, 0x810040802104080ULL, 0x4001040410900061ULL,
    0x1282040420080008ULL, 0x4340020202225000ULL, 0x8020090080800ULL, 0x1004048080800ULL,
    0x8004008412426ULL, 0x20008c8080884ULL, 0x1208084480210200ULL, 0x24a0200202004001ULL,
    0x201c0108220800c8ULL, 0x242006420901800ULL, 0x141000401011020ULL, 0x665420084240900ULL,
    0x10084400a02002a0ULL, 0x1a103008412800ULL, 0x40880210002827ULL, 0x245180a0080200a0ULL,
    0x201001031004001ULL, 0x490010020241100ULL, 0x820804094400ULL, 0x2000728022008400ULL,
    0x850041010051002ULL, 0x8022024200101028ULL, 0x1444210100102401ULL, 0x880400820060200ULL,
    0x4040400001100ULL, 0x800a009100020440ULL, 0x401044900040100ULL, 0x808008100148060ULL,
    0x34490802c0025000ULL, 0x8080b008003002ULL, 0x100420044401010ULL, 0xd000084010404200ULL,
    0x1440404101001210ULL, 0x4040080081000020ULL, 0x100801c400800400ULL, 0x1010051052801906ULL,
    0x4820841042100804ULL, 0x100208828080420ULL, 0x1010030251100400ULL, 0x8404000104880010ULL,
    0x2809202020000ULL, 0xc00801444025ULL, 0x8022221401040800ULL, 0x8202584301220008ULL,
    0x1041101011000ULL, 0x4000230061100801ULL, 0x8094200042080400ULL, 0x600040140840400ULL,
    0x4010088208841ULL, 0x805280200410a080ULL, 0x10082001241108ULL, 0x4840101200404184ULL
};

// Castling data - indexed by [color * 2 + side]
constexpr int CASTLING_KING_FROM[4] = { 4, 4, 60, 60 };
constexpr int CASTLING_KING_TO[4] = { 6, 2, 62, 58 };
constexpr int CASTLING_ROOK_FROM[4] = { 7, 0, 63, 56 };
constexpr int CASTLING_ROOK_TO[4] = { 5, 3, 61, 59 };

// OPTIMIZATION 8: Castling rights update mask - branchless update
constexpr CastlingRights CASTLING_RIGHTS_MASK[64] = {
    CastlingRights(~WHITE_OOO), ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    CastlingRights(~WHITE_CASTLE), ALL_CASTLING, ALL_CASTLING, CastlingRights(~WHITE_OO),
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    CastlingRights(~BLACK_OOO), ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    CastlingRights(~BLACK_CASTLE), ALL_CASTLING, ALL_CASTLING, CastlingRights(~BLACK_OO)
};

std::once_flag initFlag;

Bitboard slowRookAttacks(int sq, Bitboard occ) {
    Bitboard attacks = 0;
    int r = rankOf(sq), f = fileOf(sq);
    constexpr int dr[4] = {1, -1, 0, 0}, df[4] = {0, 0, 1, -1};
    for (int d = 0; d < 4; ++d) {
        for (int i = 1; i < 8; ++i) {
            int nr = r + dr[d] * i, nf = f + df[d] * i;
            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;
            int nsq = nr * 8 + nf;
            attacks |= squareBB(nsq);
            if (occ & squareBB(nsq)) break;
        }
    }
    return attacks;
}

Bitboard slowBishopAttacks(int sq, Bitboard occ) {
    Bitboard attacks = 0;
    int r = rankOf(sq), f = fileOf(sq);
    constexpr int dr[4] = {1, 1, -1, -1}, df[4] = {1, -1, 1, -1};
    for (int d = 0; d < 4; ++d) {
        for (int i = 1; i < 8; ++i) {
            int nr = r + dr[d] * i, nf = f + df[d] * i;
            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) break;
            int nsq = nr * 8 + nf;
            attacks |= squareBB(nsq);
            if (occ & squareBB(nsq)) break;
        }
    }
    return attacks;
}

Bitboard rookMask(int sq) {
    Bitboard mask = 0;
    int r = rankOf(sq), f = fileOf(sq);
    for (int i = r + 1; i < 7; ++i) mask |= squareBB(i * 8 + f);
    for (int i = r - 1; i > 0; --i) mask |= squareBB(i * 8 + f);
    for (int i = f + 1; i < 7; ++i) mask |= squareBB(r * 8 + i);
    for (int i = f - 1; i > 0; --i) mask |= squareBB(r * 8 + i);
    return mask;
}

Bitboard bishopMask(int sq) {
    Bitboard mask = 0;
    int r = rankOf(sq), f = fileOf(sq);
    for (int i = 1; r + i < 7 && f + i < 7; ++i) mask |= squareBB((r + i) * 8 + f + i);
    for (int i = 1; r + i < 7 && f - i > 0; ++i) mask |= squareBB((r + i) * 8 + f - i);
    for (int i = 1; r - i > 0 && f + i < 7; ++i) mask |= squareBB((r - i) * 8 + f + i);
    for (int i = 1; r - i > 0 && f - i > 0; ++i) mask |= squareBB((r - i) * 8 + f - i);
    return mask;
}

Bitboard indexToOccupancy(int index, Bitboard mask) {
    Bitboard occ = 0;
    while (mask) {
        int sq = popLSB(mask);
        if (index & 1) occ |= squareBB(sq);
        index >>= 1;
    }
    return occ;
}

void init() {
    std::call_once(initFlag, []() {
        // King and knight attacks
        for (int sq = 0; sq < 64; ++sq) {
            int r = rankOf(sq), f = fileOf(sq);
            
            Bitboard k = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int df = -1; df <= 1; ++df)
                    if ((dr || df) && r + dr >= 0 && r + dr < 8 && f + df >= 0 && f + df < 8)
                        k |= squareBB((r + dr) * 8 + f + df);
            KING_ATTACKS[sq] = k;
            
            Bitboard n = 0;
            constexpr int kd[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
            for (auto [dr, df] : kd)
                if (r + dr >= 0 && r + dr < 8 && f + df >= 0 && f + df < 8)
                    n |= squareBB((r + dr) * 8 + f + df);
            KNIGHT_ATTACKS[sq] = n;
            
            // Pawn attacks
            Bitboard wpawn = 0, bpawn = 0;
            if (r < 7) {
                if (f > 0) wpawn |= squareBB(sq + 7);
                if (f < 7) wpawn |= squareBB(sq + 9);
            }
            if (r > 0) {
                if (f > 0) bpawn |= squareBB(sq - 9);
                if (f < 7) bpawn |= squareBB(sq - 7);
            }
            PAWN_ATTACKS[WHITE][sq] = wpawn;
            PAWN_ATTACKS[BLACK][sq] = bpawn;
        }
        
        // Between and line bitboards
        for (int s1 = 0; s1 < 64; ++s1) {
            for (int s2 = 0; s2 < 64; ++s2) {
                BETWEEN_BB[s1][s2] = 0;
                LINE_BB[s1][s2] = 0;
                if (s1 == s2) continue;
                
                int r1 = rankOf(s1), f1 = fileOf(s1);
                int r2 = rankOf(s2), f2 = fileOf(s2);
                int dr = (r2 > r1) - (r2 < r1);
                int df = (f2 > f1) - (f2 < f1);
                
                bool onLine = (r1 == r2) || (f1 == f2) || (std::abs(r2-r1) == std::abs(f2-f1));
                if (onLine) {
                    for (int r = r1 + dr, f = f1 + df; r != r2 || f != f2; r += dr, f += df)
                        BETWEEN_BB[s1][s2] |= squareBB(r * 8 + f);
                    
                    for (int r = r1, f = f1; r >= 0 && r < 8 && f >= 0 && f < 8; r -= dr, f -= df)
                        LINE_BB[s1][s2] |= squareBB(r * 8 + f);
                    for (int r = r1 + dr, f = f1 + df; r >= 0 && r < 8 && f >= 0 && f < 8; r += dr, f += df)
                        LINE_BB[s1][s2] |= squareBB(r * 8 + f);
                }
            }
        }
        
        // Magic bitboards
        Bitboard* rookPtr = ROOK_TABLE;
        for (int sq = 0; sq < 64; ++sq) {
            auto& m = ROOK_MAGICS[sq];
            m.mask = rookMask(sq);
            m.magic = ROOK_MAGIC_NUMBERS[sq];
            m.shift = 64 - popCount(m.mask);
            m.attacks = rookPtr;
            
            int size = 1 << popCount(m.mask);
            for (int i = 0; i < size; ++i) {
                Bitboard occ = indexToOccupancy(i, m.mask);
                int idx = static_cast<int>((occ * m.magic) >> m.shift);
                m.attacks[idx] = slowRookAttacks(sq, occ);
            }
            rookPtr += size;
        }
        
        Bitboard* bishopPtr = BISHOP_TABLE;
        for (int sq = 0; sq < 64; ++sq) {
            auto& m = BISHOP_MAGICS[sq];
            m.mask = bishopMask(sq);
            m.magic = BISHOP_MAGIC_NUMBERS[sq];
            m.shift = 64 - popCount(m.mask);
            m.attacks = bishopPtr;
            
            int size = 1 << popCount(m.mask);
            for (int i = 0; i < size; ++i) {
                Bitboard occ = indexToOccupancy(i, m.mask);
                int idx = static_cast<int>((occ * m.magic) >> m.shift);
                m.attacks[idx] = slowBishopAttacks(sq, occ);
            }
            bishopPtr += size;
        }
    });
}

} // namespace Tables

//============================================================================
// Zobrist hashing - shared for repetition detection
//============================================================================
namespace Zobrist {
    inline std::array<std::array<std::array<std::uint64_t, 64>, 6>, 2> PIECE{};
    inline std::array<std::uint64_t, 16> CASTLING{};
    inline std::array<std::uint64_t, 8> EP{};
    inline std::uint64_t SIDE = 0;
    inline std::once_flag initFlag;

    inline std::uint64_t splitmix64(std::uint64_t& x) {
        std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    inline void init() {
        std::call_once(initFlag, []() {
            std::uint64_t seed = 0x123456789abcdefULL;
            for (int c = 0; c < 2; ++c) {
                for (int pt = 0; pt < 6; ++pt) {
                    for (int sq = 0; sq < 64; ++sq) {
                        PIECE[c][pt][sq] = splitmix64(seed);
                    }
                }
            }
            for (int i = 0; i < 16; ++i) CASTLING[i] = splitmix64(seed);
            for (int i = 0; i < 8; ++i) EP[i] = splitmix64(seed);
            SIDE = splitmix64(seed);
        });
    }
} // namespace Zobrist

//============================================================================
// OPTIMIZATION 9: Inline attack lookups
//============================================================================
FORCE_INLINE Bitboard rookAttacks(int sq, Bitboard occ) {
    const auto& e = Tables::ROOK_MAGICS[sq];
    return e.attacks[((occ & e.mask) * e.magic) >> e.shift];
}

FORCE_INLINE Bitboard bishopAttacks(int sq, Bitboard occ) {
    const auto& e = Tables::BISHOP_MAGICS[sq];
    return e.attacks[((occ & e.mask) * e.magic) >> e.shift];
}

FORCE_INLINE Bitboard queenAttacks(int sq, Bitboard occ) {
    return rookAttacks(sq, occ) | bishopAttacks(sq, occ);
}

//============================================================================
} // namespace chess
