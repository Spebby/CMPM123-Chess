// My attempt at "efficiently" storing chess positions without going full bitboard.
// I use some bitscanning to efficiently extract data from bitboards.
#include <cstring>
#include "ProtoBoard.h"
#include "BitFunctions.h"

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
    forEachBit([&pos](uint8_t index) {
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
    forEachBit([&pos](uint8_t index) {
        pos.push_back(index);
    }, bits[pieceToBoard(piece)]);

    return pos;
}

// Micro Optimisation: Consider ordering it to check WPawn, BPawn, WKnight, BKnight.. etc first.
ChessPiece ProtoBoard::PieceFromIndex(const uint8_t index) const {
    uint64_t mask = (1ULL << index);

    for (int i = 0; i < 12; i++) {
        if ((bits[i] & mask) != 0) {
            return PieceFromProtoIndex(i);
        }
    }

    return (ChessPiece)0;
}