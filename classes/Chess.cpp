#include <cmath>
#include <stdexcept>

#include "Chess.h"
#include "MagicBitboards/MagicBitboards.h"

#include "ChessAI.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

#define currState _state.top()

const int MAX_DEPTH = 3;

Chess::Chess() {
	initMagicBitboards();

	// TODO: Let player set this by hand.
	_gameOps.AIPlayer = 1;
}

Chess::~Chess() {
	cleanupMagicBitboards();
}

const int spriteSize = 64;
// make a chess piece for the player
ChessBit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece) {
	const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };
	ChessBit* bit = new ChessBit();

	// we could maybe cache this to make things simpler.
	const char* pieceName = pieces[piece - 1];
	std::string spritePath = std::string("chess/") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
	bit->LoadTextureFromFile(spritePath.c_str());
	bit->setOwner(getPlayerAt(playerNumber));

	/*	Sebastian opted to use the 4th and 5th bit to denote if a piece is black or white,
		but this seems like a bit of an oversight on his part, and it arguably makes more sense
		in the context of this code to simply use the 4th bit to denote the color of a piece.
		*/
	bit->setGameTag(playerNumber << 3 | piece);
	bit->setSize(spriteSize, spriteSize);

	return bit;
}

// we DON'T error check here b/c of overhead. Check it yourself!
ChessBit* Chess::PieceForPlayer(const char piece) {
	return PieceForPlayer((int)!std::isupper(piece), pieceFromSymbol.at(std::tolower(piece)));
}

Move* Chess::MoveForPositions(const int from, const int to) {
	for (Move& move : _currentMoves) {
		if (move.getFrom() == from && move.getTo() == to) {
			return &move;
		}
	}

	return nullptr;
}

void Chess::setUpBoard() {
	setNumberOfPlayers(2);

	// It upsets me greatly that, as far as I can tell, a double loop is unfortunately required.
	for (int file = _gameOps.Y - 1; file >= 0; file--) {
		for (int rank = 0; rank < _gameOps.X; rank++) {
			// Unfortunately the _gameOps.Y - y part is neccesary to get this to display properly.
			_grid[file * 8 + rank].initHolder((ImVec2(static_cast<float>(rank * 64 + 50),
											   static_cast<float>((_gameOps.Y - file) * 64 + 50))),
									"chess/square.png", rank, file);
			// game tag init to 0
			// notation is set later.
		}
	}

	if (gameHasAI()) {
		setAIPlayer(_gameOps.AIPlayer);
	}

	// Seems like a good idea to start the game using Fen notation, so I can easily try different states for testing.
	setStateString("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");	// Standard setup
    // setStateString("r3k2r/pp2bpp1/2np3p/2p1p2K/P6q/1PP5/3P1n1P/8");				// checkmate check
    // setStateString("5k2/8/8/3q4/8/8/5PPP/7K"); endTurn(); 						// debug checkmate, black checkmate in one
    // setStateString("6k1/1P3ppp/8/8/8/8/8/4K3");									// check pawn promotion
    // setStateString("r1bq1rk1/p3bppp/2q2n2/4p3/2B1P3/2N1BQ2/PPP3PP/R3K2R");		// can king castle on queen side with square under attack
    // setStateString("r1bqkbnr/ppp2ppp/2n5/3pQ3/8/5N1P/PPP1PPP1/RNBQKB1R");		// black in check -- Invalid state. FEN should throw an error.
    // setStateString("Q7/3k4/5K1p/5PpP/6P1/8/8/8");								// white to move but makes illegal move
    // setStateString("5k2/2R5/8/3Q3p/3p4/3P1K2/4P3/8");							// white can checkmate
    // setStateString("r1bn3r/pp3ppp/5k2/q1pP4/2N5/P1QB4/1Q3PPP/R3K2R");			// didn't see check
    // setStateString("r3k1r1/pp1N1ppp/2n1pq2/1B6/3QP3/8/P2Q1PPP/R3K2R");			// didn't see check
	// setStateString("k7/8/8/8/2Q5/8/3K4/2r5"); // Rook Check Sanity Check
	// setStateString("k7/8/8/8/2Q5/8/3K4/2b5"); // Bishop Check Sanity Check

	// Great website for generating test cases
	// http://www.netreal.de/Forsyth-Edwards-Notation/index.php

	startGame();
	_currentMoves = MoveGenerator(currState, false);
}


// ================================ Engine Functions ================================

// nothing for chess
// Consider adding support for clicking on highlighted positions to allow insta moving. Could be cool.
bool Chess::actionForEmptyHolder(BitHolder &holder) {
	return false;
}

bool Chess::canBitMoveFrom(Bit& bit, BitHolder& src) {
	// un-lit the squares when clicking on a new square.
	clearPositionHighlights();

	ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
	bool canMove = false;
	const int i = srcSquare.getIndex();

	for (const Move& move : _currentMoves) {
		if (move.getFrom() == i) {
			canMove = true;
			uint8_t attacking = move.getTo();
			_grid[attacking].setMoveHighlighted(true);
			_litSquare.push(&_grid[attacking]);
		}
	}
	return canMove;
}

// Is the piece allowed to move here?
bool Chess::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
	ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
	ChessSquare& dstSquare = static_cast<ChessSquare&>(dst);
	const uint8_t i = srcSquare.getIndex();
	const uint8_t j = dstSquare.getIndex();
	for (const Move& move : _currentMoves) {
		if (move.getFrom() == i && move.getTo() == j) {
			return true;
		}
	}

	return false;
}

// borrow graeme's code; note, game calls this function and unless we want to call base we'll need to specifically end turn here.
void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
	ChessSquare& srcSquare = static_cast<ChessSquare&>(src);
	ChessSquare& dstSquare = static_cast<ChessSquare&>(dst);

	// get the move being played
	const uint8_t i = srcSquare.getIndex();
	const uint8_t j = dstSquare.getIndex();

	// this line currently garauntees that we'll auto turn into a queen b/c queen promotion option is always pushed first.
	// eventually when I make a gui for it, we'll need to revise this to handle there being multiple "moves" for a single position.
	Move* move = MoveForPositions(i, j);

	if (!move) {
		throw std::runtime_error("Illegal Move attempted ft: " + std::to_string(i) + " " + std::to_string(j));
	}

	// EnPassant Check
	if (currState.getEnPassantSquare() == j && ((bit.gameTag() & 7) == ChessPiece::Pawn)) {
		_grid[j + (currState.isBlackTurn() ? 8 : -8)].destroyBit();
		// increment score.
	} else if (move->isCastle() && ((bit.gameTag() & 7) == ChessPiece::King)) { // castle
		uint8_t offset = currState.isBlackTurn() ? 56 : 0;
		uint8_t rookSpot = (move->QueenSideCastle() ? 0 : 7) + offset;
		uint8_t targ = (move->QueenSideCastle() ? 3 : 5) + offset;
		_grid[targ].setBit(_grid[rookSpot].bit());
		_grid[rookSpot].setBit(nullptr);
	} else if (move->isPromotion()) {
		// todo, but for the moment b/c of how our move is selected, queen will be only "move" we can make.
		int newPiece = 0;
		switch(move->getFlags() & Move::FlagCodes::Promotion) {
			case Move::FlagCodes::ToQueen:
				newPiece = ChessPiece::Queen;
				break;
			case Move::FlagCodes::ToKnight:
				newPiece = ChessPiece::Knight;
				break;
			case Move::FlagCodes::ToRook:
				newPiece = ChessPiece::Rook;
				break;
			case Move::FlagCodes::ToBishop:
				newPiece = ChessPiece::Bishop;
				break;
		}

		// awesome cast
		dstSquare.setBit(PieceForPlayer(currState.isBlackTurn(), (ChessPiece)newPiece));
	}

	// check if we took a rook
	_state.emplace(currState, *move);

	// do some check to prompt the UI to select a promotion.

	// call base.
	Game::bitMovedFromTo(bit, src, dst);

	clearPositionHighlights();
}

inline void Chess::clearPositionHighlights() {
	while (!_litSquare.empty()) {
		_litSquare.top()->setMoveHighlighted(false);
		_litSquare.pop();
	}
}

// free all the memory used by the game on the heap
void Chess::stopGame() {
	for (int i = 0; i < 64; i++) {
		_grid[i].destroyBit();
	}
}

void Chess::endTurn() {
	_currentMoves = MoveGenerator(currState, false);

	// Game checks for if game is over in super.
	Game::endTurn();

	if (gameHasAI() && getCurrentPlayer() && getCurrentPlayer()->isAIPlayer()) {
		// If AI is next, we don't need to cache player moves.
		updateAI();
		return;
	}

	#ifdef DEBUG
	Loggy.log(stateString());
	#endif
}

Player* Chess::checkForWinner() {
	// check to see if either player has won
	if (_currentMoves.empty() && Chess::InCheck()) {
		return getInactivePlayer();
	}

	return nullptr;
}

bool Chess::checkForDraw() {
	// check to see if the board is full

	// Stalemate
	if (_currentMoves.empty() && !Chess::InCheck()) {
		return true;
	}

	// 50 Move counter
	if (currState.getHalfClock() == 50) {
		return true;
	}

	// TODO: 3-fold-repetition

	return false;
}

/**
 * When doing threading, a thread-safe transposition table is neccesary
 * 
 * threefoldRepetitionList
 * 
 * Filter Illegal Moves(?)
 */


// ========================== Misc AI ==========================

const int inf = 999999UL;

// this is the function that will be called by the AI
void Chess::updateAI() {
	// run negamax on all avalible positions.
	int bestVal = -inf;
	const Move* bestMove = nullptr;

	#ifdef DEBUG
	Loggy.log("Starting AI Occuancy: " + std::to_string(currState.getOccupancyBoard()));
	#endif

	// these values are inverted b/c creating the newstate will effectively the values anyways.
	const int player = currState.isBlackTurn() ? 1 : -1;
	for (const Move& move : _currentMoves) {
		ChessAI newState = ChessAI(GameState(currState, move));
		#ifdef DEBUG
		//uint64_t bit = newState.logDebugInfo();
		#endif
		//newState.reserveMemoryStack(MAX_DEPTH);
		int moveVal = -newState.negamax(MAX_DEPTH, 0, -inf, inf, player);
		
		if (moveVal > bestVal) {
			bestMove = &move;
			bestVal  = moveVal;
		}
	}


	// Currently the AI does pick a move to play, but this does not update the GUI.
	// Make sure to reflect this change to the player.

	// Part of this is because of how UpdateAI is being called... Revise EndTurn() to work as intended.
	if (bestMove) {
		#ifdef DEBUG
		Loggy.log("AI Picked Move: " + ChessSquare::indexToPosNotation(bestMove->getFrom()) + ", " + ChessSquare::indexToPosNotation(bestMove->getTo()));
		#endif
		// TODO: Chess Square dropBitAtPoint
		auto& fromSquare = _grid[bestMove->getFrom()];
		auto& toSquare = _grid[bestMove->getTo()];

		auto fromBit = _grid[bestMove->getFrom()].bit();
		auto toPosition = toSquare.getPosition();

		toSquare.dropBitAtPoint(fromBit, toPosition);
		fromSquare.setBit(nullptr);
		bitMovedFromTo(*fromSquare.bit(), fromSquare, toSquare);
	}
}

/*
if (_dropTarget && _dropTarget->dropBitAtPoint(_dragBit, _dragBit->getPosition())) {
	// Yes, notify the interested parties:
	_dragBit->setPickedUp(false);
	_dragBit->setPosition(_dropTarget->getPosition()); // don't animate
	if (_oldHolder)
		_oldHolder->draggedBitTo(_dragBit, _dropTarget);
	// I'm pretty sure _oldHolder will always be null here.
	bitMovedFromTo(*_dragBit, *_oldHolder, *_dropTarget);
}
*/