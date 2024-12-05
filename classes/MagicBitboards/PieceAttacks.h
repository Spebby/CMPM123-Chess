#ifndef PIECE_ATTACKS_H
#define PIECE_ATTACKS_H

#include <stdint.h>

// Pre-calculated knight attack bitboards
const uint64_t KnightAttacks[64] = {
    0x20400ULL, 0x50800ULL, 0xa1100ULL, 0x142200ULL,
    0x284400ULL, 0x508800ULL, 0xa01000ULL, 0x402000ULL,
    0x2040004ULL, 0x5080008ULL, 0xa110011ULL, 0x14220022ULL,
    0x28440044ULL, 0x50880088ULL, 0xa0100010ULL, 0x40200020ULL,
    0x204000402ULL, 0x508000805ULL, 0xa1100110aULL, 0x1422002214ULL,
    0x2844004428ULL, 0x5088008850ULL, 0xa0100010a0ULL, 0x4020002040ULL,
    0x20400040200ULL, 0x50800080500ULL, 0xa1100110a00ULL, 0x142200221400ULL,
    0x284400442800ULL, 0x508800885000ULL, 0xa0100010a000ULL, 0x402000204000ULL,
    0x2040004020000ULL, 0x5080008050000ULL, 0xa1100110a0000ULL, 0x14220022140000ULL,
    0x28440044280000ULL, 0x50880088500000ULL, 0xa0100010a00000ULL, 0x40200020400000ULL,
    0x204000402000000ULL, 0x508000805000000ULL, 0xa1100110a000000ULL, 0x1422002214000000ULL,
    0x2844004428000000ULL, 0x5088008850000000ULL, 0xa0100010a0000000ULL, 0x4020002040000000ULL,
    0x400040200000000ULL, 0x800080500000000ULL, 0x1100110a00000000ULL, 0x2200221400000000ULL,
    0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010a000000000ULL, 0x2000204000000000ULL,
    0x4020000000000ULL, 0x8050000000000ULL, 0x110a0000000000ULL, 0x22140000000000ULL,
    0x44280000000000ULL, 0x88500000000000ULL, 0x10a00000000000ULL, 0x20400000000000ULL
};

// Pre-calculated king attack bitboards
const uint64_t KingAttacks[64] = {
    0x302ULL, 0x705ULL, 0xe0aULL, 0x1c14ULL,
    0x3828ULL, 0x7050ULL, 0xe0a0ULL, 0xc040ULL,
    0x30203ULL, 0x70507ULL, 0xe0a0eULL, 0x1c141cULL,
    0x382838ULL, 0x705070ULL, 0xe0a0e0ULL, 0xc040c0ULL,
    0x3020300ULL, 0x7050700ULL, 0xe0a0e00ULL, 0x1c141c00ULL,
    0x38283800ULL, 0x70507000ULL, 0xe0a0e000ULL, 0xc040c000ULL,
    0x302030000ULL, 0x705070000ULL, 0xe0a0e0000ULL, 0x1c141c0000ULL,
    0x3828380000ULL, 0x7050700000ULL, 0xe0a0e00000ULL, 0xc040c00000ULL,
    0x30203000000ULL, 0x70507000000ULL, 0xe0a0e000000ULL, 0x1c141c000000ULL,
    0x382838000000ULL, 0x705070000000ULL, 0xe0a0e0000000ULL, 0xc040c0000000ULL,
    0x3020300000000ULL, 0x7050700000000ULL, 0xe0a0e00000000ULL, 0x1c141c00000000ULL,
    0x38283800000000ULL, 0x70507000000000ULL, 0xe0a0e000000000ULL, 0xc040c000000000ULL,
    0x302030000000000ULL, 0x705070000000000ULL, 0xe0a0e0000000000ULL, 0x1c141c0000000000ULL,
    0x3828380000000000ULL, 0x7050700000000000ULL, 0xe0a0e00000000000ULL, 0xc040c00000000000ULL,
    0x203000000000000ULL, 0x507000000000000ULL, 0xa0e000000000000ULL, 0x141c000000000000ULL,
    0x2838000000000000ULL, 0x5070000000000000ULL, 0xa0e0000000000000ULL, 0x40c0000000000000ULL
};

// Pre-compute pawn bitboard attacks
constexpr uint64_t GPAttack(uint8_t square, bool isWhite) {
    uint64_t bitboard = 0;

    int file = square % 8;
    int rank = square / 8;

    if (isWhite) {
        if (file > 0 && rank < 7) bitboard |= (1ULL << (square + 7)); // Forward-left
        if (file < 7 && rank < 7) bitboard |= (1ULL << (square + 9)); // Forward-right
    } else {
        if (file > 0 && rank > 0) bitboard |= (1ULL << (square - 9)); // Backward-left
        if (file < 7 && rank > 0) bitboard |= (1ULL << (square - 7)); // Backward-right
    }

    return bitboard;
}

constexpr uint64_t PawnAttacks[64][2] = {
    {GPAttack(0, true), GPAttack(0, false)},   {GPAttack(1, true), GPAttack(1, false)},
    {GPAttack(2, true), GPAttack(2, false)},   {GPAttack(3, true), GPAttack(3, false)},
    {GPAttack(4, true), GPAttack(4, false)},   {GPAttack(5, true), GPAttack(5, false)},
    {GPAttack(6, true), GPAttack(6, false)},   {GPAttack(7, true), GPAttack(7, false)},

    {GPAttack(8, true), GPAttack(8, false)},   {GPAttack(9, true), GPAttack(9, false)},
    {GPAttack(10, true), GPAttack(10, false)}, {GPAttack(11, true), GPAttack(11, false)},
    {GPAttack(12, true), GPAttack(12, false)}, {GPAttack(13, true), GPAttack(13, false)},
    {GPAttack(14, true), GPAttack(14, false)}, {GPAttack(15, true), GPAttack(15, false)},

    {GPAttack(16, true), GPAttack(16, false)}, {GPAttack(17, true), GPAttack(17, false)},
    {GPAttack(18, true), GPAttack(18, false)}, {GPAttack(19, true), GPAttack(19, false)},
    {GPAttack(20, true), GPAttack(20, false)}, {GPAttack(21, true), GPAttack(21, false)},
    {GPAttack(22, true), GPAttack(22, false)}, {GPAttack(23, true), GPAttack(23, false)},

    {GPAttack(24, true), GPAttack(24, false)}, {GPAttack(25, true), GPAttack(25, false)},
    {GPAttack(26, true), GPAttack(26, false)}, {GPAttack(27, true), GPAttack(27, false)},
    {GPAttack(28, true), GPAttack(28, false)}, {GPAttack(29, true), GPAttack(29, false)},
    {GPAttack(30, true), GPAttack(30, false)}, {GPAttack(31, true), GPAttack(31, false)},

    {GPAttack(32, true), GPAttack(32, false)}, {GPAttack(33, true), GPAttack(33, false)},
    {GPAttack(34, true), GPAttack(34, false)}, {GPAttack(35, true), GPAttack(35, false)},
    {GPAttack(36, true), GPAttack(36, false)}, {GPAttack(37, true), GPAttack(37, false)},
    {GPAttack(38, true), GPAttack(38, false)}, {GPAttack(39, true), GPAttack(39, false)},

    {GPAttack(40, true), GPAttack(40, false)}, {GPAttack(41, true), GPAttack(41, false)},
    {GPAttack(42, true), GPAttack(42, false)}, {GPAttack(43, true), GPAttack(43, false)},
    {GPAttack(44, true), GPAttack(44, false)}, {GPAttack(45, true), GPAttack(45, false)},
    {GPAttack(46, true), GPAttack(46, false)}, {GPAttack(47, true), GPAttack(47, false)},

    {GPAttack(48, true), GPAttack(48, false)}, {GPAttack(49, true), GPAttack(49, false)},
    {GPAttack(50, true), GPAttack(50, false)}, {GPAttack(51, true), GPAttack(51, false)},
    {GPAttack(52, true), GPAttack(52, false)}, {GPAttack(53, true), GPAttack(53, false)},
    {GPAttack(54, true), GPAttack(54, false)}, {GPAttack(55, true), GPAttack(55, false)},

    {GPAttack(56, true), GPAttack(56, false)}, {GPAttack(57, true), GPAttack(57, false)},
    {GPAttack(58, true), GPAttack(58, false)}, {GPAttack(59, true), GPAttack(59, false)},
    {GPAttack(60, true), GPAttack(60, false)}, {GPAttack(61, true), GPAttack(61, false)},
    {GPAttack(62, true), GPAttack(62, false)}, {GPAttack(63, true), GPAttack(63, false)}
};

#endif // PIECE_ATTACKS_H
