#pragma once

#include <iostream>
#include <iomanip>
#include <cstdint>
#include "magicbitboards.h"

// Pretty print a bitboard in 8x8 format with rank/file labels
void printBitboard(const char* title, uint64_t bitboard) {
    std::cout << "\n" << title << ":\n";
    std::cout << "  +---+---+---+---+---+---+---+---+\n";
    
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << " |";
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            std::cout << " " << (bitboard & (1ULL << square) ? "X" : " ") << " |";
        }
        std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "    a   b   c   d   e   f   g   h\n";
}

// Print multiple related bitboards side by side
void printBitboardRow(const char* title, const std::vector<uint64_t>& boards) {
    // Print title
    std::cout << "\n" << title << ":\n";
    
    // Print top border
    for (size_t i = 0; i < boards.size(); i++) {
        std::cout << "  +---+---+---+---+---+---+---+---+  ";
    }
    std::cout << "\n";
    
    // Print board contents
    for (int rank = 7; rank >= 0; rank--) {
        // Print rank number for each board
        for (size_t i = 0; i < boards.size(); i++) {
            std::cout << rank + 1 << " |";
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                std::cout << " " << (boards[i] & (1ULL << square) ? "X" : " ") << " |";
            }
            std::cout << "  ";
        }
        std::cout << "\n";
        
        // Print horizontal line
        for (size_t i = 0; i < boards.size(); i++) {
            std::cout << "  +---+---+---+---+---+---+---+---+  ";
        }
        std::cout << "\n";
    }
    
    // Print file letters for each board
    for (size_t i = 0; i < boards.size(); i++) {
        std::cout << "    a   b   c   d   e   f   g   h      ";
    }
    std::cout << "\n";
}

/*
int main() {
    // Initialize magic bitboards
    initMagicBitboards();
    
    // Test some interesting positions
    
    // 1. Knight moves from e4
    int e4 = SQUARE(3, 4);  // rank 4, file e
    printBitboard("Knight moves from e4", KnightAttacks[e4]);
    
    // 2. King moves from e4
    printBitboard("King moves from e4", KingAttacks[e4]);
    
    // 3. Rook moves with different blockers
    uint64_t emptyBoard = 0ULL;
    uint64_t someBlockers = (1ULL << SQUARE(3, 6)) |  // g4
                           (1ULL << SQUARE(5, 4)) |  // e6
                           (1ULL << SQUARE(3, 2));   // c4
    
    std::vector<uint64_t> rookBoards = {
        getRookAttacks(e4, emptyBoard),
        getRookAttacks(e4, someBlockers)
    };
    printBitboardRow("Rook moves from e4 (empty board vs blockers)", rookBoards);
    
    // 4. Bishop moves with different blockers
    std::vector<uint64_t> bishopBoards = {
        getBishopAttacks(e4, emptyBoard),
        getBishopAttacks(e4, someBlockers)
    };
    printBitboardRow("Bishop moves from e4 (empty board vs blockers)", bishopBoards);
    
    // 5. Queen moves (combination of rook and bishop)
    std::vector<uint64_t> queenBoards = {
        getQueenAttacks(e4, emptyBoard),
        getQueenAttacks(e4, someBlockers)
    };
    printBitboardRow("Queen moves from e4 (empty board vs blockers)", queenBoards);
    
    // 6. Show some blocking positions
    printBitboard("Blocking pieces", someBlockers);
    
    // 7. Demonstrate pawn attacks
    uint64_t whitePawn = 1ULL << e4;
    uint64_t blackPawn = 1ULL << SQUARE(4, 4);  // e5
    
    std::vector<uint64_t> pawnBoards = {
        WHITE_PAWN_ATTACKS(whitePawn),
        BLACK_PAWN_ATTACKS(blackPawn)
    };
    printBitboardRow("Pawn attacks (white e4 vs black e5)", pawnBoards);
    
    // Cleanup
    cleanupMagicBitboards();
    return 0;
}*/