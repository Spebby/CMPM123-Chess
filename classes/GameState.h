#pragma once

#include <cstdint>

#include "ProtoBoard.h"
#include "Move.h"

class GameState {
	public:
	// Generate from FEN
	GameState(const ProtoBoard& board, const bool isBlack, const uint8_t castling,
		const uint8_t enTarget, const uint8_t hClock, const uint16_t fClock,
		const uint8_t wKingSquare, const uint8_t bKingSquare);
	
	// generate next move
	GameState(const GameState& old, const Move& move);

	GameState(const GameState& old);

	GameState& operator=(const GameState& other);
	bool operator==(const GameState& other);

	bool isBlackTurn() const { return isBlack; }
	uint8_t getEnPassantSquare()	const { return enPassantSquare; }
	uint8_t getCastlingRights()	const { return castlingRights; }
	uint8_t getHalfClock()		const { return halfClock; }
	uint16_t getClock() const { return clock; }
	void setCastlingRights(const uint8_t rights) { castlingRights = rights; }
	ProtoBoard& getProtoBoard() { return bits; }
	//const char* getState() const { return state; }

	uint8_t getEnemyKingSquare()    const { return enemyKingSquare; }
	uint8_t getFriendlyKingSquare() const { return friendlyKingSquare; }

	uint64_t getSlidingAttackBoard() const;

	uint64_t getFriendlyOccuupancySquare() const { return isBlack ? bits.getBlackOccupancyBoard() : bits.getWhiteOccupancyBoard(); }
	uint64_t getEnemyOccuupancySquare()    const { return isBlack ? bits.getWhiteOccupancyBoard() : bits.getBlackOccupancyBoard(); }

	// TODO: init state w/ proper values. For later!

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

	//char state[64];

	// TODO: add a way of tracking board state (graeme recommends a 64 bit char array)

	// if i want to support undoing, it may be a good idea to add a string to these (or an associated class) that keeps track
	// of FEN so I can reload from FEN.
};