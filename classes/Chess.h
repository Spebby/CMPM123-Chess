#pragma once

#include <vector>
#include <unordered_map>
#include <stack>

#include "Game.h"
#include "ChessSquare.h"
#include "GameState.h"
#include "ChessPiece.h"

typedef std::unordered_map<uint8_t, std::vector<Move>> MoveTable;

// the main game class
class Chess : public Game {
public:
	Chess();
	~Chess();

	// set up the board
	void		setUpBoard() override;

	void		endTurn() override;
	Player*		checkForWinner() override;
	bool		checkForDraw() override;
	std::string	initialStateString() override;
	std::string	stateString() override;
	void		setStateString(const std::string &fen) override;
	bool		actionForEmptyHolder(BitHolder& holder) override;
	bool		canBitMoveFrom(Bit& bit, BitHolder& src) override;
	bool		canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
	void		bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

	static MoveTable MoveGenerator(GameState&, bool=false);
	static bool isPinned(int);
	static bool isMovingAlongRay(int, int, int);
	static bool squareIsInCheckRay(int);

	void		stopGame() override;
	BitHolder&	getHolderAt(const int x, const int y) override { return _grid[y * 8 + x]; }

	void		updateAI() override;
	bool		gameHasAI() override { return true; }

	// we only use this in application.cpp for debugging purposes
	MoveTable getMoves() const { return _currentMoves; }
	GameState getState() const { return _state.top(); }

private:
	ChessBit* 		PieceForPlayer(const int playerNumber, ChessPiece piece);
	ChessBit* 		PieceForPlayer(const char piece);
	Move*			MoveForPositions(const int i, const int j);
	const char		bitToPieceNotation(int rank, int file) const;
    const char		bitToPieceNotation(int i) const;
	inline void 	clearPositionHighlights();

	static void CalculateAttackData(GameState&);
	static void GeneratePawnMoves(MoveTable&, GameState&);
	static inline void GeneratePawnPush(MoveTable&,   GameState&, const uint64_t, const uint8_t);
	static inline void GeneratePawnAttack(MoveTable&, GameState&, const uint64_t, const uint8_t); // helper
	static void GenerateKnightMoves(MoveTable&,  GameState&);
	static void GenerateSlidingMoves(MoveTable&, GameState&);
	static inline void GenerateSlidingMovesHelper(MoveTable&, const std::function<uint64_t(uint8_t, uint64_t)>&, const uint64_t&, const uint64_t&, const uint64_t&, const uint64_t&);
	static void GenerateKingMoves(MoveTable&, GameState&);

	static bool draw(const MoveTable&, bool);

	ChessSquare	_grid[64];
	std::stack<GameState> _state;

	// the non-AI player's moves. We cache this as our agnostic backend can't be modified to support passing a move list
	// directly (nor should it). For player turns specifically, the engine running at 100% efficientcy is overkill.
	MoveTable _currentMoves;
	// I don't need to use a stack, a vector would be perfectly fine, but a stack is syntactically simpler.
	std::stack<ChessSquare*> _litSquare;
};