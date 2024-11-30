#pragma once

#include <vector>
#include <unordered_map>
#include <stack>

#include "Game.h"
#include "ChessSquare.h"
#include "GameState.h"
#include "ChessPiece.h"

// the main game class
class Chess : public Game {
public:
	Chess();
	~Chess();

	// set up the board
	void		setUpBoard() override;

	Player*		checkForWinner() override;
	bool		checkForDraw() override;
	std::string	initialStateString() override;
	std::string	stateString() override;
	void		setStateString(const std::string &fen) override;
	bool		actionForEmptyHolder(BitHolder& holder) override;
	bool		canBitMoveFrom(Bit& bit, BitHolder& src) override;
	bool		canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
	void		bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

	void MoveGenerator(bool=false);
	bool isPinned(int);
	bool isMovingAlongRay(int, int, int);
	bool squareIsInCheckRay(int);


	void		stopGame() override;
	BitHolder&	getHolderAt(const int x, const int y) override { return _grid[y * 8 + x]; }

	void		updateAI() override;
	bool		gameHasAI() override { return false; }

	// we only use this in application.cpp for debugging purposes
	std::unordered_map<uint8_t, std::vector<Move>> getMoves() const { return _moves; }
	GameState getState() const { return _state.top(); }

private:
	ChessBit* 		PieceForPlayer(const int playerNumber, ChessPiece piece);
	ChessBit* 		PieceForPlayer(const char piece);
	Move*			MoveForPositions(const int i, const int j);
	const char		bitToPieceNotation(int rank, int file) const;
    const char		bitToPieceNotation(int i) const;
	inline void 	clearPositionHighlights();

	void GeneratePawnMoves();
	void GenerateKnightMoves();
	void GenerateBishopMoves();
	void GenerateRookMoves();
	void GenerateQueenMoves();
	void GenerateKingMoves();

	void CalculateAttackData();

	// distances at a given position to the board's boundries. North, East, South, West, NE, SE, SW, NW
	int _dist[64][8];
	ChessSquare	_grid[64];
	std::unordered_map<uint8_t, std::vector<Move>> _moves;
	std::stack<GameState> _state;
	// I don't need to use a stack, a vector would be perfectly fine, but a stack is syntactically simpler.
	std::stack<ChessSquare*> _litSquare;
};