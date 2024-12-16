#include "MagicBitboards.h"
#include "BitFunctions.h"

// Attack lookup tables
static uint64_t* RAttacks[64];
static uint64_t* BAttacks[64];

// Size of attack tables for each square
static const int RAttackSize[64] = {
    4096, 2048, 2048, 2048, 2048, 2048, 2048, 4096,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    2048, 1024, 1024, 1024, 1024, 1024, 1024, 2048,
    4096, 2048, 2048, 2048, 2048, 2048, 2048, 4096
};

static const int BAttackSize[64] = {
    64, 32, 32, 32, 32, 32, 32, 64,
    32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 128, 128, 128, 128, 32, 32,
    32, 32, 128, 512, 512, 128, 32, 32,
    32, 32, 128, 512, 512, 128, 32, 32,
    32, 32, 128, 128, 128, 128, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32,
    64, 32, 32, 32, 32, 32, 32, 64
};

// Helper functions
#pragma warning(push)
#pragma warning(disable : 4146)
static inline uint64_t indexToUint64(int index, int bits, uint64_t m) {
    uint64_t result = 0ULL;
    for (int i = 0; i < bits; i++) {
        uint64_t least_bit = m & -m;
        if (index & (1 << i)) {
            result |= least_bit;
        }
        m &= (m - 1);
    }
    return result;
}
#pragma warning(pop)

static uint64_t ratt(int sq, uint64_t block) {
    uint64_t result = 0ULL;
    int rk = sq / 8, fl = sq % 8, r, f;

    for (r = rk + 1; r <= 7; r++) {
        result |= (1ULL << (fl + r * 8));
        if (block & (1ULL << (fl + r * 8))) break;
    }
    for (r = rk - 1; r >= 0; r--) {
        result |= (1ULL << (fl + r * 8));
        if (block & (1ULL << (fl + r * 8))) break;
    }
    for (f = fl + 1; f <= 7; f++) {
        result |= (1ULL << (f + rk * 8));
        if (block & (1ULL << (f + rk * 8))) break;
    }
    for (f = fl - 1; f >= 0; f--) {
        result |= (1ULL << (f + rk * 8));
        if (block & (1ULL << (f + rk * 8))) break;
    }
    return result;
}

static uint64_t batt(int sq, uint64_t block) {
    uint64_t result = 0ULL;
    int rk = sq / 8, fl = sq % 8, r, f;

    for (r = rk + 1, f = fl + 1; r <= 7 && f <= 7; r++, f++) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk - 1, f = fl + 1; r >= 0 && f <= 7; r--, f++) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk - 1, f = fl - 1; r >= 0 && f >= 0; r--, f--) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk + 1, f = fl - 1; r <= 7 && f >= 0; r++, f--) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    return result;
}

// Public interface implementations
uint64_t getRookAttacks(int square, uint64_t occupied) {
    occupied &= RMasks[square];
    occupied *= RMagic[square];
    occupied >>= RShifts[square];
    return RAttacks[square][occupied];
}

uint64_t getBishopAttacks(int square, uint64_t occupied) {
    occupied &= BMasks[square];
    occupied *= BMagic[square];
    occupied >>= BShifts[square];
    return BAttacks[square][occupied];
}

uint64_t getQueenAttacks(int square, uint64_t occupied) {
    return getRookAttacks(square, occupied) | getBishopAttacks(square, occupied);
}

void initMagicBitboards(void) {
    for (int square = 0; square < 64; square++) {
        // Initialize rook attacks
        RAttacks[square] = new uint64_t[RAttackSize[square]];
        uint64_t mask = RMasks[square];
        int bits = popCount(mask);
        int n = 1 << bits;

        for (int i = 0; i < n; i++) {
            uint64_t subset = indexToUint64(i, bits, mask);
            uint64_t index = (subset * RMagic[square]) >> RShifts[square];
            RAttacks[square][index] = ratt(square, subset);
        }

        // Initialize bishop attacks
        BAttacks[square] = new uint64_t[BAttackSize[square]];
        mask = BMasks[square];
        bits = popCount(mask);
        n = 1 << bits;

        for (int i = 0; i < n; i++) {
            uint64_t subset = indexToUint64(i, bits, mask);
            uint64_t index = (subset * BMagic[square]) >> BShifts[square];
            BAttacks[square][index] = batt(square, subset);
        }
    }
}

void cleanupMagicBitboards(void) {
    for (int square = 0; square < 64; square++) {
        delete[] RAttacks[square];
        delete[] BAttacks[square];
    }
}
