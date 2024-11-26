// My attempt at "efficiently" storing chess positions without going full bitboard.
// I use some bitscanning to efficiently extract data from bitboards.
#pragma once

#include <vector>
#include <cstdint>
#include "ChessPiece.h"

class ProtoBoard {
    public:
    ProtoBoard();

    // copy constructor
    ProtoBoard(const ProtoBoard& other);
    ProtoBoard& operator=(const ProtoBoard& other);

    bool operator==(const ProtoBoard& other);

    inline void set(const ChessPiece piece, const bool isBlack, const uint64_t board) {
        bits[pieceToBoard(piece, isBlack)] = board;
    }

    inline void enable(const ChessPiece piece, const bool isBlack, const int pos) {
        bits[pieceToBoard(piece, isBlack)] |=  (1ULL << pos);
    }

    inline void disable(const ChessPiece piece, const bool isBlack, const int pos) {
        bits[pieceToBoard(piece, isBlack)] &= ~(1ULL << pos);
    }

    uint64_t getOccupancyBoard() const;
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

    private:
    inline int pieceToBoard(ChessPiece piece, bool isBlack) const {
        return isBlack ? (piece + 5) : piece - 1;
    }

    inline int pieceToBoard(ChessPiece piece) const {
        return (piece & 8) ? ((piece & 7) + 5) : (piece & 7) - 1;
    }


    template <typename Func>
    void getBitPositions(Func func, uint64_t bitboard) const;

    inline uint8_t bitScanForward(uint64_t bb) const {
    #if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
    #else
        return __builtin_ffsll(bb) - 1;
    #endif
    };

    // https://www.chessprogramming.org/Bitboards
    // 0-5 is White,   6 - 11 is Black.
    uint64_t bits[12];
};