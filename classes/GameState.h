#pragma once

#include <cstdint>
#include <stack>

#include "MagicBitboards/ProtoBoard.h"
#include "Move.h"

// TODO read these
// https://www.chessprogramming.org/Repetitions
// http://www.open-chess.org/viewtopic.php?f=3&t=2209

// 2 bytes... safe to pack.
// Keeps track of disposable memory.
#pragma pack(push, 1)
struct GameStateMemory {
	uint8_t halfClock;
	// 4 bits in reality, but to pack efficently on all systems, half a byte screws with this.
	uint8_t castlingRights;
	ChessPiece capturedPieceType;

	GameStateMemory(uint8_t hc, uint8_t cr, ChessPiece cpt) : halfClock(hc), castlingRights(cr), capturedPieceType(cpt) {}
};
#pragma pack(pop)

class GameState {
	public:
	// Generate from FEN
	GameState(const ProtoBoard& board, const bool isBlack, const uint8_t castling,
		const uint8_t enTarget, const uint8_t hClock, const uint16_t fClock,
		const uint8_t wKingSquare, const uint8_t bKingSquare);
	
	// generate next move
	GameState(const GameState& old, const Move& move);

	GameState(const GameState& old);

	GameState& operator=(const GameState&);
	bool operator==(const GameState&);

	void MakeMove(const Move&);
	void UnmakeMove(const Move&, const GameStateMemory&);
	// TODO: We're going to need to figure out a proper way to restore
	// TODO: rights when unmaking, since keeping track of castling rights is pretty
	// TODO: important, same w/ half clock.

	GameStateMemory makeMemoryState() {
		// hoping RVO kicks in.
		GameStateMemory memory = GameStateMemory(halfClock, castlingRights, capturedPieceType);
		return memory;
	}

	// this is really funny so i will be using this syntax
	void operator+(const Move& move) {
		MakeMove(move);
	}

	bool isBlackTurn() const { return isBlack; }
	uint8_t getEnPassantSquare()	const { return enPassantSquare; }
	uint8_t getCastlingRights()	const { return castlingRights; }
	uint8_t getHalfClock()		const { return halfClock; }
	uint16_t getClock() const { return clock; }
	void setCastlingRights(const uint8_t rights) { castlingRights = rights; }
	// consider not allowing direct access to protoboard
	ProtoBoard& getProtoBoard() { return bits; }

	uint8_t getEnemyKingSquare()    const { return enemyKingSquare; }
	uint8_t getFriendlyKingSquare() const { return friendlyKingSquare; }

	ChessPiece PieceFromIndex(const uint8_t index) const { return bits.PieceFromIndex(index); }

	uint64_t getOccupancyBoard() const { return bits.getOccupancyBoard(); }
	uint64_t getFriendlyOccuupancyBoard() const { return isBlack ? bits.getBlackOccupancyBoard() : bits.getWhiteOccupancyBoard(); }
	uint64_t getEnemyOccuupancyBoard()    const { return isBlack ? bits.getWhiteOccupancyBoard() : bits.getBlackOccupancyBoard(); }

	const uint64_t& getPieceOccupancyBoard(ChessPiece piece, bool isBlack) const { return bits[isBlack ? (piece + 5) : piece - 1]; }

	protected:
	// I wanted a class that managed muh bits a bit nicer than a c-string, and one that
	// made use of some more advanced techniques like BitScan, without going full in on bitboards.
	ProtoBoard bits;
	bool isBlack;
	uint8_t castlingRights : 4; // KQkq
	// FOR FUTURE SELF: I am not capping this b/c I need an easy to check null value (ex, 255)
	uint8_t enPassantSquare;
	uint8_t halfClock : 6;
	uint16_t clock;
	//const uint64_t hash;
	uint8_t friendlyKingSquare : 6;
	uint8_t enemyKingSquare : 6;
	ChessPiece capturedPieceType;
};