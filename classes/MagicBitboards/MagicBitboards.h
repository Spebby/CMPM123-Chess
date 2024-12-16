#pragma once

#ifndef MAGIC_BITBOARDS_H
#define MAGIC_BITBOARDS_H

#include <stdint.h>

// Function declarations
void initMagicBitboards(void);
void cleanupMagicBitboards(void);

// Bitboard manipulation macros
#define SET_BIT(bb, sq) ((bb) |= (1ULL << (sq)))
#define CLEAR_BIT(bb, sq) ((bb) &= ~(1ULL << (sq)))
#define GET_BIT(bb, sq) ((bb) & (1ULL << (sq)))
#define SQUARE(rank, file) ((rank) * 8 + (file))

// Directional shift macros
#define NORTH(bb) ((bb) << 8)
#define SOUTH(bb) ((bb) >> 8)
#define EAST(bb) (((bb) & ~0x8080808080808080ULL) << 1)
#define WEST(bb) (((bb) & ~0x0101010101010101ULL) >> 1)
#define NORTH_EAST(bb) (((bb) & ~0x8080808080808080ULL) << 9)
#define NORTH_WEST(bb) (((bb) & ~0x0101010101010101ULL) << 7)
#define SOUTH_EAST(bb) (((bb) & ~0x8080808080808080ULL) >> 7)
#define SOUTH_WEST(bb) (((bb) & ~0x0101010101010101ULL) >> 9)

// Pawn attack macros
#define WHITE_PAWN_ATTACKS(pawns) (NORTH_EAST(pawns) | NORTH_WEST(pawns))
#define BLACK_PAWN_ATTACKS(pawns) (SOUTH_EAST(pawns) | SOUTH_WEST(pawns))

// Magic numbers and shift amounts
#include "MagicNumbers.h" // Contains RMagic[], BMagic[], RShifts[], BShifts[]
#include "AttackMasks.h" // Contains RMasks[], BMasks[]
#include "PieceAttacks.h" // Contains KnightAttacks[], KingAttacks[], PawnAttacks[][]

// Attack lookup functions
uint64_t getRookAttacks(int square, uint64_t occupied);
uint64_t getBishopAttacks(int square, uint64_t occupied);
uint64_t getQueenAttacks(int square, uint64_t occupied);

#endif // MAGIC_BITBOARDS_H
