// My attempt at "efficiently" storing chess positions without going full bitboard.
// I use some bitscanning to efficiently extract data from bitboards.
#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "../ChessPiece.h"

class ProtoBoard {
    public:
    ProtoBoard();

    // copy constructor
    ProtoBoard(const ProtoBoard& other);
    ProtoBoard& operator=(const ProtoBoard& other);

    bool operator==(const ProtoBoard& other);

    // modify
    uint64_t& operator[](size_t index) {
        if (index >= 12) {
            throw std::out_of_range("Index out of bounds");
        }
        return bits[index];
    }

    // read-only
    const uint64_t& operator[](size_t index) const {
        if (index >= 12) {
            throw std::out_of_range("Index out of bounds");
        }
        return bits[index];
    }

    inline void set(const ChessPiece piece, const bool isBlack, const uint64_t board) {
        bits[pieceToBoard(piece, isBlack)] = board;
    }

    inline void enable(const ChessPiece piece, const bool isBlack, const int pos) {
        bits[pieceToBoard(piece, isBlack)] |=  (1ULL << pos);
    }

    inline void disable(const ChessPiece piece, const bool isBlack, const int pos) {
        bits[pieceToBoard(piece, isBlack)] &= ~(1ULL << pos);
    }

    inline uint64_t getOccupancyBoard() const { return bits[0] | bits[1] | bits[2] | bits[3] | bits[4]  | bits[5] 
                                                | bits[6] | bits[7] | bits[8] | bits[9] | bits[10] | bits[11]; }
    inline uint64_t getWhiteOccupancyBoard() const { return bits[0] | bits[1] | bits[2] | bits[3] | bits[4]  | bits[5]; }
    inline uint64_t getBlackOccupancyBoard() const { return bits[6] | bits[7] | bits[8] | bits[9] | bits[10] | bits[11]; }

    uint64_t& getBitBoard(ChessPiece piece, bool isBlack);
    std::vector<uint8_t> getBitPositions(ChessPiece piece, bool isBlack) const;

    // GameTag versions
    inline void set(const ChessPiece piece, const uint64_t board) {
        bits[pieceToBoard(piece)] = board;
    }

    inline void enable(const ChessPiece piece, const int pos) {
        bits[pieceToBoard(piece)] |=  (1ULL << pos);
    }

    inline void disable(const ChessPiece piece, const int pos) {
        bits[pieceToBoard(piece)] &= ~(1ULL << pos);
    }

    uint64_t& getBitBoard(ChessPiece piece);
    std::vector<uint8_t> getBitPositions(ChessPiece piece) const;
    ChessPiece PieceFromIndex(uint8_t index) const;

    static inline ChessPiece PieceFromProtoIndex(int i) {
        if (i < 6) {
            return (ChessPiece)(i + 1);
            // 1 - 6 for white pieces
        }

        return (ChessPiece)(i + 3);
        // 9 - 14 for black pieces
    }

    private:
    inline int pieceToBoard(ChessPiece piece, bool isBlack) const {
        return isBlack ? (piece + 5) : piece - 1;
    }

    inline int pieceToBoard(ChessPiece piece) const {
        return (piece & 8) ? ((piece & 7) + 5) : (piece & 7) - 1;
    }

    // https://www.chessprogramming.org/Bitboards
    // 0-5 is White,   6 - 11 is Black.
    uint64_t bits[12];
};