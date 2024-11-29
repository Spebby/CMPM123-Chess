// My attempt at "efficiently" storing chess positions without going full bitboard.
// I use some bitscanning to efficiently extract data from bitboards.
#include <cstring>
#include "ProtoBoard.h"

ProtoBoard::ProtoBoard() {
    std::memset(bits, 0, sizeof(bits));
}

// copy constructor
ProtoBoard::ProtoBoard(const ProtoBoard& other) {
    std::memcpy(bits, other.bits, sizeof(bits));
}

ProtoBoard& ProtoBoard::operator=(const ProtoBoard& other) {
    if (this != &other) {
        std::memcpy(bits, other.bits, sizeof(bits));
    }
    return *this;
}

bool ProtoBoard::operator==(const ProtoBoard& other) {
    for (int i = 0; i < 12; i++) {
        if (bits[i] != other.bits[i]) {
            return false;
        }
    }

    return true;
}

uint64_t& ProtoBoard::getBitBoard(ChessPiece piece, bool isBlack) {
    return bits[pieceToBoard(piece, isBlack)];
}

std::vector<uint8_t> ProtoBoard::getBitPositions(ChessPiece piece, bool isBlack) const {
    std::vector<uint8_t> pos;
    getBitPositions([&pos](uint8_t index) {
        pos.push_back(index);
    }, bits[pieceToBoard(piece, isBlack)]);

    return pos;
}

// GameTag versions
uint64_t& ProtoBoard::getBitBoard(ChessPiece piece) {
    return bits[pieceToBoard(piece)];
}

std::vector<uint8_t> ProtoBoard::getBitPositions(ChessPiece piece) const {
    std::vector<uint8_t> pos;
    getBitPositions([&pos](uint8_t index) {
        pos.push_back(index);
    }, bits[pieceToBoard(piece)]);

    return pos;
}

ChessPiece ProtoBoard::PieceFromIndex(uint8_t index) const {
    uint64_t mask = (1ULL << index);

    for (int i = 0; i < 12; i++) {
        if ((bits[i] & mask) != 0) {
            if (i < 6) {
                return (ChessPiece)(i + 1);
                // 1 - 6 for white pieces
            } else {
                return (ChessPiece)(i + 3);
                // 9 - 14 for black pieces
            }
        }
    }

    return (ChessPiece)0;
}

// privates
template <typename Func>
void ProtoBoard::getBitPositions(Func func, uint64_t bitboard) const {
    while (bitboard) {
        uint8_t index = bitScanForward(bitboard);
        func(index);
        bitboard &= bitboard - 1;
    }
}