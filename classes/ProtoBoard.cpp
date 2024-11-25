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

uint64_t ProtoBoard::getOccupancyBoard() const {
    return bits[0] | bits[1] | bits[2] | bits[3] | bits[4]  | bits[5] 
            | bits[6] | bits[7] | bits[8] | bits[9] | bits[10] | bits[11];
}

uint64_t& ProtoBoard::getBitBoard(ChessPiece piece, bool isBlack) {
    return bits[pieceToBoard(piece, isBlack)];
}

std::vector<uint8_t> ProtoBoard::getBitPositions(ChessPiece piece, bool isBlack) const {
    return getBitPositions(bits[pieceToBoard(piece, isBlack)]);
}

// GameTag versions
uint64_t& ProtoBoard::getBitBoard(ChessPiece piece) {
    return bits[pieceToBoard(piece)];
}

std::vector<uint8_t> ProtoBoard::getBitPositions(ChessPiece piece) const {
    return getBitPositions(bits[pieceToBoard(piece)]);
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
std::vector<uint8_t> ProtoBoard::getBitPositions(uint64_t bitboard) const {
    std::vector<uint8_t> positions;
    while (bitboard) {
        // TODO: Ask Graeme if this is a safe way to BitScan!!
        unsigned long index = __builtin_ctzll(bitboard);
        positions.push_back(static_cast<int>(index));
        bitboard &= bitboard - 1; // Clear the least significant set bit
    }

    return positions;
}