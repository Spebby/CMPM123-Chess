#pragma once

#include <cstdint>

static inline uint8_t bitScanForward(uint64_t bb) {
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long index;
    _BitScanForward64(&index, bb);
    return static_cast<uint8_t>(index);
#else
    return __builtin_ffsll(bb) - 1;
#endif
};

// Compiler-specific bit manipulation functions
#ifdef __clang__
	// Clang/LLVM specific bit counting
	static inline int popCount(uint64_t b) {
		return __builtin_popcountll(b);
	}

	// Find first set bit (returns 0-63, undefined for b==0)
	static inline int getFirstBit(uint64_t b) {
		return __builtin_ctzll(b);
	}

#elif defined(_MSC_VER)
    #include <intrin.h>
    static inline int popCount(uint64_t b) {
        return static_cast<int>(__popcnt64(b));
    }

    // Find first set bit (returns 0-63, undefined for b==0)
    static inline int getFirstBit(uint64_t b) {
        unsigned long index;
        _BitScanForward64(&index, b); // Intrinsic for MSVC
        return static_cast<int>(index);
    }

#else
	// Fallback bit counting implementation
	static inline int popCount(uint64_t b) {
		int r = 0;
		while (b) {
			r++;
			b &= b - 1;
		}
		return r;
	}

	// Fallback first bit implementation
	static inline int getFirstBit(uint64_t b) {
		const int BitTable[64] = {
			63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34,
			61, 29, 2, 51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35,
			62, 31, 40, 4, 49, 5, 52, 26, 60, 6, 23, 44, 46, 27, 56, 16,
			7, 39, 48, 24, 59, 14, 12, 55, 38, 28, 58, 20, 37, 17, 36, 8
		};
		uint64_t debruijn = 0x03f79d71b4cb0a89ULL;
		return BitTable[((b ^ (b-1)) * debruijn) >> 58];
	}
#endif

template <typename Func>
static void forEachBit(Func func, uint64_t bitboard) {
    while (bitboard) {
        uint8_t index = bitScanForward(bitboard);
        func(index);
        bitboard &= bitboard - 1;
    }
}