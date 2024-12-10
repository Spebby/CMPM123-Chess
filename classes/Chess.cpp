#include <cmath>
#include <stdexcept>
#include "Chess.h"
#include "MagicBitboards/MagicBitboards.h"
#include "MagicBitboards/BitFunctions.h"
#include "../tools/Logger.h"
#include "PrecomputedData.h"

#include "ChessAI.h"

#define currState _state.top()

const int maxDepth = 3;

Chess::Chess() {
	initMagicBitboards();
	// TODO: replace _state with vector & manage it like it's a stack internally.
	// ^ this is probably not neccesary. For the AI specifically, it may be possible to insert directly into a longer vector, while still keeping
	// the hash table for convenience when workign w/ player moves.
}

Chess::~Chess() {
	cleanupMagicBitboards();
}

const std::map<char, ChessPiece> pieceFromSymbol = {
	{'p', ChessPiece::Pawn},
	{'n', ChessPiece::Knight},
	{'b', ChessPiece::Bishop},
	{'r', ChessPiece::Rook},
	{'q', ChessPiece::Queen},
	{'k', ChessPiece::King}
};

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

Move* Chess::MoveForPositions(const int i, const int j) {
	for (unsigned int k = 0; k < _playerMoves[i].size(); k++) {
		if (_playerMoves[i][k].getTo() == j) {
			return &_playerMoves[i][k];
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
	setStateString("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // setStateString("r3k2r/pp2bpp1/2np3p/2p1p2K/P6q/1PP5/3P1n1P/8");  // checkmate check
    // setStateString("5k2/8/8/3q4/8/8/5PPP/7K"); endTurn(); // debug checkmate, black checkmate in one
    // setStateString("6k1/1P3ppp/8/8/8/8/8/4K3"); // check pawn promotion
    // setStateString("r1bq1rk1/p3bppp/2q2n2/4p3/2B1P3/2N1BQ2/PPP3PP/R3K2R");        // can king castle on queen side with square under attack
    // setStateString("r1bqkbnr/ppp2ppp/2n5/3pQ3/8/5N1P/PPP1PPP1/RNBQKB1R"); // black in check
    // setStateString("Q7/3k4/5K1p/5PpP/6P1/8/8/8"); // white to move but makes illegal move
    // setStateString("5k2/2R5/8/3Q3p/3p4/3P1K2/4P3/8"); // white can checkmate
    // setStateString("r1bn3r/pp3ppp/5k2/q1pP4/2N5/P1QB4/1Q3PPP/R3K2R"); // didn't see check
    // setStateString("r3k1r1/pp1N1ppp/2n1pq2/1B6/3QP3/8/P2Q1PPP/R3K2R"); // didn't see check
	startGame();
	_playerMoves = MoveGenerator(currState, false);
}

// ================================ Move Generation ================================

// I'll probably have to move these into move generator so I can recursively call move generator
bool inCheck;
bool pinned;
bool doubleCheck;
bool pinInPosition;
uint8_t friendlyKingSquare;
uint64_t checkRayBitmask;
uint64_t pinRayBitmask;
uint64_t attackMap;
const int dir[8] = {8, 1, -8, -1, 9, -7, -9, 7};
bool generateQuiets;

inline void ReInitGen() {
	attackMap = 0ULL;
	inCheck = false;
	pinned = false;
	doubleCheck = false;
	pinInPosition = false;
	checkRayBitmask = 0U;
	pinRayBitmask = 0U;
}

MoveTable Chess::MoveGenerator(GameState& state, bool capturesOnly) {
	// this isn't optimised the best; in the future we'll want to use bitboards instead.
	generateQuiets = !capturesOnly;

	MoveTable table;
	// reset variables.
	ReInitGen();
	friendlyKingSquare = state.getFriendlyKingSquare();
	CalculateAttackData(state);
	Loggy.log("Attack Map - " + std::to_string(attackMap));

	GenerateKingMoves(table, state);

	if (doubleCheck) {
		return table;
	}

	GenerateSlidingMoves(table, state);
	GenerateKnightMoves(table, state);
	GeneratePawnMoves(table, state);

	return table;
}

inline int getDirectionOffset(int a, int b) {
    // Convert linear index a and b to (row, col)
    int row_a = a / 8;
    int col_a = a % 8;
    int row_b = b / 8;
    int col_b = b % 8;

    // Calculate the differences in row and column
    int delta_row = row_b - row_a;
    int delta_col = col_b - col_a;

	// These numbers are the directions the index's of _dist and dir represent. 
	const int NORTH = 0;
	const int EAST = 1;
	const int SOUTH = 2;
	const int WEST = 3;
	const int NORTH_EAST = 4;
	const int SOUTH_EAST = 5;
	const int SOUTH_WEST = 6;
	const int NORTH_WEST = 7;

    if (delta_col == 0) {
		return delta_row > 0 ? NORTH : SOUTH;
	}

	if (delta_row == 0) {
		return delta_col > 0 ? EAST : WEST;
	}

	if (delta_col > 0) {
		return delta_row > 0 ? NORTH_EAST : SOUTH_EAST;
	}
	else {
		return delta_row > 0 ? NORTH_WEST : SOUTH_WEST;
	}
}

void Chess::CalculateAttackData(GameState& state) {
	bool blackIsEnemy = !state.isBlackTurn();
	// update sliding attack lanes
	uint64_t occupancy = state.getOccupancyBoard();
	
	{
		uint64_t rooks = state.getPieceOccupancyBoard(ChessPiece::Rook, blackIsEnemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getRookAttacks(square, occupancy);
		}, rooks);

		uint64_t bishops = state.getPieceOccupancyBoard(ChessPiece::Bishop, blackIsEnemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getBishopAttacks(square, occupancy);
		}, bishops);

		uint64_t queens = state.getPieceOccupancyBoard(ChessPiece::Queen, blackIsEnemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getQueenAttacks(square, occupancy);
		}, queens);
	}
	Loggy.log(Logger::WARNING, "Sliding Step - " + std::to_string(attackMap));

	// check around king for pins
	uint64_t kingAdjacentDanger = KingAttacks[friendlyKingSquare] & attackMap;
	uint64_t friendly = state.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t square) {
		// we don't need to search any more if we're already in doubleCheck.
		// This is up here b/c I can't break out of the bits method in a single statement, so program
		// control would return to top and go through whole loop again.
		if (doubleCheck) {
			return;
		}

		int direction = getDirectionOffset(friendlyKingSquare, square);
		int n = _dist[friendlyKingSquare][direction];
		int dirOffset = dir[direction];
		bool friendlyBlock = false;
		uint64_t rayMask = 0;
		bool onDiagonal = direction > 3;

		for (int i = 0; i < n; i++) {
			uint8_t cur = friendlyKingSquare + dirOffset * (i + 1);
			rayMask |= 1ULL << cur;

			ChessPiece piece = state.PieceFromIndex(square);
			if (piece == NoPiece) continue;

			bool friendlyPiece = (friendly & (1ULL << square)) != 0;
			if (friendlyPiece) {
				if (!friendlyBlock) {
					friendlyBlock = true;
					continue;
				}

				break;
			}

			// Enemy piece here
			if ((onDiagonal && IsDiagonalPiece(piece)) || (!onDiagonal && IsHorizontalPiece(piece))) {
				// Friendly piece blocks, so it is pinned
				if (friendlyBlock) {
					pinInPosition = true;
					pinRayBitmask |= rayMask;
				} else {
					// no block, so we're in check.
					checkRayBitmask |= rayMask;
					doubleCheck = inCheck;
					inCheck = true;
				}
				break;
			} else {
				break;
				// piece cannot attack king, blocking any potential pins/checks
			}
		}

	// Since attackMap currently only has sliding data, any piece adjacent to the king that's in danger
	// is likely to threaten the king. So we filter out rays that are obviously safe.
	}, kingAdjacentDanger);

	uint8_t enemyKingSquare = state.getEnemyKingSquare();

	// king attacks
	attackMap |= KingAttacks[enemyKingSquare];
	Loggy.log(Logger::WARNING, "King Step - " + std::to_string(attackMap));

	// Knight Attacks
	uint64_t knights = state.getPieceOccupancyBoard(ChessPiece::Knight, blackIsEnemy);
	bool isKnightCheck = false;
	uint64_t knightAttackMap = 0;
	forEachBit([&](uint8_t fromSquare) {
		uint64_t attacks = KnightAttacks[fromSquare];
		knightAttackMap |= attacks;
		// if knight is attacking king, update doubleCheck status.
		if (!isKnightCheck && ((knightAttackMap & (1ULL << friendlyKingSquare)) != 0)) {
			isKnightCheck = true;
			doubleCheck = inCheck;
			inCheck = true;
			checkRayBitmask |= 1ULL << fromSquare;
		}
	}, knights);

	attackMap |= knightAttackMap;

	// Pawn Attacks
	uint64_t pawns = state.getPieceOccupancyBoard(ChessPiece::Pawn, blackIsEnemy);
	bool pawnCheck = false;
	uint64_t pawnAttackMap = 0;
	forEachBit([&](uint8_t fromSquare) {
		uint64_t attacks = PawnAttacks[fromSquare][blackIsEnemy];
		pawnAttackMap |= attacks;

		if (!pawnCheck && (pawnAttackMap & friendlyKingSquare) != 0) {
			pawnCheck = true;
			doubleCheck = inCheck;
			inCheck = true;
			checkRayBitmask |= (1ULL << fromSquare);
		}
	}, pawns);

	attackMap |= pawnAttackMap;

	// TODO: saving king, pawn, knight into attack map not neccesary,
	// TODO: 

	// TODO: add check to attack map for potential checks after enpassant

	Loggy.log(Logger::WARNING, "Pawn Step - " + std::to_string(attackMap));
	Loggy.log(Logger::WARNING, "Check Ray - " + std::to_string(checkRayBitmask));
	Loggy.log(Logger::WARNING, "Pin Ray   - " + std::to_string(pinRayBitmask));
}

void Chess::GeneratePawnMoves(MoveTable& moves, GameState& state) {
	uint64_t pawns = state.getPieceOccupancyBoard(ChessPiece::Pawn, state.isBlackTurn());
	const uint64_t occupancy = state.getOccupancyBoard();
	const uint64_t enemies   = state.getEnemyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		GeneratePawnPush(moves, state, occupancy, fromSquare);
		GeneratePawnAttack(moves, state, enemies, fromSquare);
	}, pawns);
}

inline void Chess::GeneratePawnPush(MoveTable& moves, GameState& state, const uint64_t occupancy, const uint8_t fromSquare) {
	if (inCheck && isPinned(fromSquare)) return;
	int moveDir = state.isBlackTurn() ? dir[2] : dir[0];
	uint8_t toSquare = fromSquare + moveDir;
	bool canPromote = toSquare == (state.isBlackTurn() ? (fromSquare % 8) : (fromSquare % 8) + 56);
	bool canDPush = state.isBlackTurn() ? ((fromSquare / 8) == 6) : ((fromSquare / 8) == 1);
	
	if ((occupancy & (1ULL << toSquare)) == 0) {
		// Make sure that we continue to block if pushing. 
		if (inCheck && !squareIsInCheckRay(toSquare)) return;
		if (canPromote) {
			for (int i = 0; i < 4; i++) {
				moves[fromSquare].emplace_back(fromSquare, toSquare, Move::FlagCodes::ToQueen << i);
			}
		} else {
			moves[fromSquare].emplace_back(fromSquare, toSquare);
			toSquare += moveDir;
			if (canDPush && ((occupancy & (1ULL << toSquare)) == 0)) {
				moves[fromSquare].emplace_back(fromSquare, toSquare, Move::FlagCodes::DoublePush);
			}
		}
	}
}

inline void Chess::GeneratePawnAttack(MoveTable& moves, GameState& state, const uint64_t enemies, const uint8_t fromSquare) {
	uint64_t attacks = PawnAttacks[fromSquare][state.isBlackTurn()] & enemies;
	//& checkRayBitmask;
	forEachBit([&](uint8_t toSquare) {
		if (inCheck && !squareIsInCheckRay(toSquare)) return;

		bool EnPassant = state.getEnPassantSquare() == toSquare;
		// if enpassant square is specified, then we know it's a legal move b/c en passant square is set on previous turn.
		moves[fromSquare].emplace_back(fromSquare, toSquare, EnPassant ? Move::FlagCodes::EnCapture : 0);
	}, attacks);
}

void Chess::GenerateKnightMoves(MoveTable& moves, GameState& state) {
	// all non-pinned knights
	uint64_t knights = state.getPieceOccupancyBoard(ChessPiece::Knight, state.isBlackTurn()) & ~pinRayBitmask;
	const uint64_t friends   = state.getFriendlyOccuupancyBoard();
	const uint64_t moveMask  = ~friends;
	//& checkRayBitmask;

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare)) return;

		uint64_t attacks = KnightAttacks[fromSquare] & moveMask;
		moves[fromSquare].reserve(popCount(attacks));
		forEachBit([&](uint8_t toSquare) {
			// if generating only captures, or this is not a blocking move while in check, break.
			if ((inCheck && !squareIsInCheckRay(toSquare))) return;
			// skip if in check & knight is not going to block/capture attacking piece
			moves[fromSquare].emplace_back(fromSquare, toSquare);
		}, attacks);
	}, knights);
}

#include <functional>

// TODO: consider merging sliding moves down to simplify things. Queen doesn't need her own step.
void Chess::GenerateSlidingMoves(MoveTable& moves, GameState& state) {
	const uint64_t queens    = state.getPieceOccupancyBoard(ChessPiece::Queen,  state.isBlackTurn());
	uint64_t cardinals 		 = state.getPieceOccupancyBoard(ChessPiece::Rook,   state.isBlackTurn()) | queens;
	uint64_t ordinals 		 = state.getPieceOccupancyBoard(ChessPiece::Bishop, state.isBlackTurn()) | queens;

	if (inCheck) {
		cardinals &= ~pinRayBitmask;
		ordinals  &= ~pinRayBitmask;
	}

	const uint64_t occupancy = state.getOccupancyBoard();
	const uint64_t enemies   = state.getEnemyOccuupancyBoard();

	// TODO: optimisation can be made here if we are only interested in non-quiet moves
	const uint64_t moveMask  = (~occupancy | enemies);
	//& checkRayBitmask;

	GenerateSlidingMovesHelper(moves, getRookAttacks,  cardinals, occupancy, enemies, moveMask);
	GenerateSlidingMovesHelper(moves, getBishopAttacks, ordinals, occupancy, enemies, moveMask);
}

void Chess::GenerateSlidingMovesHelper(MoveTable& moves, const std::function<uint64_t(uint8_t, uint64_t)>& getAttacksFunc, const uint64_t& pieceMap, const uint64_t& occupancy, const uint64_t& enemies, const uint64_t& moveMask) {
	forEachBit([&](uint8_t fromSquare) {
		uint64_t attacks = getAttacksFunc(fromSquare, occupancy) & moveMask;

		// If pinned, we can only move along that ray.
		if (isPinned(fromSquare)) {
			attacks &= ColinearMask[fromSquare][friendlyKingSquare];
		}

		// if in check, attack only the pieces we 
		if (inCheck) {
			attacks &= checkRayBitmask;
		}

		forEachBit([&](uint8_t toSquare) {
			moves[fromSquare].emplace_back(fromSquare, toSquare);
		}, attacks);
	}, pieceMap);
}

void Chess::GenerateKingMoves(MoveTable& moves, GameState& state) {
	bool black = state.isBlackTurn();
	const uint64_t enemies   = state.getEnemyOccuupancyBoard();
	const uint64_t friends   = state.getFriendlyOccuupancyBoard();
	const uint64_t attacks   = KingAttacks[friendlyKingSquare] & ~(attackMap | friends);

	forEachBit([&](uint8_t toSquare) {
		bool capture = (enemies & (1ULL << toSquare));
		uint16_t flags = Move::FlagCodes::Castling;

		// If this is not a capture...
		if (!capture) {
			// Can't go to dangerous square unless is capturing that piece
			// TODO: skip if not generating quiet moves
			if(squareIsInCheckRay(toSquare)) {
				return;
			}
		}

		// This space is safe for the king to move to
		if (!(attackMap & (1ULL << toSquare))) {
			moves[friendlyKingSquare].emplace_back(friendlyKingSquare, toSquare, flags);
		}
	}, attacks);

	bool canCastleKingSide  = (state.getCastlingRights() & (black ? 0b1000 : 0b0010)) != 0;
	bool canCastleQueenSide = (state.getCastlingRights() & (black ? 0b0100 : 0b0001)) != 0;

	// castling
	if (!inCheck && canCastleKingSide) {
		const uint64_t occupancy = state.getOccupancyBoard();
		// KingSide, f1, f8
		const uint64_t blockers = attackMap | occupancy;
		if (canCastleKingSide) {
			const uint64_t mask = state.isBlackTurn() ? BitMasks::BlackKingsideMask : BitMasks::WhiteKingsideMask;
			if ((mask & blockers) == 0) {
				const uint8_t castleSquare = friendlyKingSquare + 1;
				moves[friendlyKingSquare].emplace_back(friendlyKingSquare, castleSquare, Move::FlagCodes::Castling);
			}
		// QueenSide, d1, d8
		} else if (canCastleQueenSide) {
			const uint64_t mask      = state.isBlackTurn() ? BitMasks::BlackQueensideMask      : BitMasks::WhiteQueensideMask;
			const uint64_t blockMask = state.isBlackTurn() ? BitMasks::BlackQueensideBlockMask : BitMasks::WhiteQueensideBlockMask;
			if (((mask & blockers) == 0) && ((blockMask & occupancy) == 0)) {
				const uint8_t castleSquare = friendlyKingSquare - 2;
				moves[friendlyKingSquare].emplace_back(friendlyKingSquare, castleSquare, Move::FlagCodes::Castling);
			}
		}
	}
}

bool Chess::isPinned(int index) {
	return pinInPosition && ((pinRayBitmask >> index) & 1) != 0;
}

bool Chess::isMovingAlongRay(int asile, int startSquare, int targetSquare) {
	int moveDir = dir[targetSquare - startSquare + 63];
	return (asile == moveDir || -asile == moveDir);
}

// Helper for checking 
// TODO: with clever use of checkRayBitmask this funciton is entierly uneeded.
bool Chess::squareIsInCheckRay(int square) {
	return inCheck && ((checkRayBitmask >> square) & 1) != 0;
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

	if (_playerMoves.count(i)) {
		canMove = true;
		for (Move move : _playerMoves[i]) {
			uint8_t attacking = move.getTo();
			_grid[attacking].setMoveHighlighted(true);
			_litSquare.push(&_grid[attacking]);
			//Loggy.log("Pushed to lit: " + std::to_string(attacking));
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
	for (Move move : _playerMoves[i]) {
		if (move.getTo() == j) {
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
	if (currState.getEnPassantSquare() == j) {
		_grid[j + (currState.isBlackTurn() ? 8 : -8)].destroyBit();
		// increment score.
	} else if (move->isCastle() && ((bit.gameTag() & ChessPiece::King) == ChessPiece::King)) { // castle
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

}

void Chess::endTurn() {
	Game::endTurn();
	if (gameHasAI()) {
		// If AI is next, we don't need to cache player moves.
		if (_gameOps.AIPlayer == (_turns.size() % 2)) {
			updateAI();
			return;
		}
	}

	_playerMoves = MoveGenerator(currState, false);
	Loggy.log(stateString());
}

Player* Chess::checkForWinner() {
	// check to see if either player has won
	return nullptr;
}

bool Chess::checkForDraw() {
	// check to see if the board is full
	return false;
}

// state strings
std::string Chess::initialStateString() {
	return stateString();
}

// this still needs to be tied into imguis init and shutdown
// we will read the state string and store it in each turn object
std::string Chess::stateString() {
	std::string s;
	int emptyCount = 0;

	int file = 7, rank = 0;
	for (int i = 0; i < _gameOps.size; i++) {
		char piece = _grid[file * 8 + rank].getPieceNotation();
		rank++;

		if (piece == '0') { // Empty square
			emptyCount++;
		} else {
			if (emptyCount > 0) {
				s += std::to_string(emptyCount); // Append the count of empty squares
				emptyCount = 0; // Reset count
			}
			s += piece; // Append the piece notation
		}
		
		// Handle row breaks for FEN notation
		if ((i + 1) % 8 == 0) {
			if (emptyCount > 0) {
				s += std::to_string(emptyCount); // Append remaining empty squares at end of row
				emptyCount = 0;
			}
			if (i != (_gameOps.size - 1)) {
				s += '/'; // Add row separator
				rank = 0;
				file--;
			}
		}
	}

	s += getCurrentPlayer()->playerNumber() ? " b " : " w ";
	std::string castlingRights;
	{
		uint8_t rights = currState.getCastlingRights();
		if (rights != 0) {
			if (rights & 0b1000) castlingRights += 'K';
			if (rights & 0b0100) castlingRights += 'Q';
			if (rights & 0b0010) castlingRights += 'k';
			if (rights & 0b0001) castlingRights += 'q';
		} else {
			castlingRights += '-';
		}
	}
	s += castlingRights;

	{
		uint8_t enP = currState.getEnPassantSquare();
		if (enP < 64) {
			s += ' ' + _grid[enP].getPositionNotation() + ' ';
		} else {
			s += " - ";
		}
	}

	s += std::to_string((int)(currState.getHalfClock())) + ' ' + std::to_string((int)(currState.getClock()));

	return s;
}

// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini file and set the game state to the last saved state
// modified from Sebastian Lague's Coding Adventure on Chess. 2:37
void Chess::setStateString(const std::string& fen) {
	ProtoBoard board;
	uint8_t wKingSquare;
	uint8_t bKingSquare;

	size_t i = 0;
	{ int file = 7, rank = 0;
	for (; i < fen.size(); i++) {
		const char symbol = fen[i];
		if (symbol == ' ') { // terminating when reaching turn indicator
			break;
		}

		if (symbol == '/') {
			rank = 0;
			file--;
		} else {
			// this is for the gap syntax.
			if (std::isdigit(symbol)) {
				rank += symbol - '0';
			} else { // there is a piece here
				// b/c white is considered as "0" elsewhere in the code, it makes
				// more sense to specifically check ifBlack, even if FEN has it the
				// other way around.
				if (symbol == 'K') {
					wKingSquare = file * 8 + rank;
				} else if (symbol == 'k') {
					bKingSquare = file * 8 + rank;
				}

				int isBlack = !std::isupper(symbol);
				ChessPiece piece = pieceFromSymbol.at(std::tolower(symbol));
				_grid[file * 8 + rank].setBit(PieceForPlayer(isBlack, piece));
				board.enable(piece, isBlack, file * 8 + rank);
				rank++;
			}
		}
	}}

	i++;
	if (i >= fen.size()) {
		_state.emplace(board, 0, 0b1111, 255, 0, 0, wKingSquare, bKingSquare);
		return;
	}

	// extract the game state part of FEN
	bool isBlack = (fen[i] == 'b');
	i += 2;

	uint8_t castling = 0;
	while (i < fen.size() && fen[i] != ' ') {
		switch (fen[i++]) {
			case 'K': castling |= 1 << 3; break;
			case 'Q': castling |= 1 << 2; break;
			case 'k': castling |= 1 << 1; break;
			case 'q': castling |= 1; break;
			case '-': castling  = 0; break;
		}
	}
	i++;

	uint8_t enTarget = 255;
	if (fen[i] != '-') {
		int col	= fen[i++] - 'a';
		int row	= fen[i++] - '1';

		// Combine both to form a unique 8-bit value (8 * row + column)
		enTarget = (row << 3) | col;
	}
	i++;
	
	uint8_t  hClock = 0;
	uint16_t fClock = 0;
	while (std::isdigit(fen[++i])) { hClock = hClock * 10 + (fen[i] - '0'); }
	while (std::isdigit(fen[++i])) { fClock = fClock * 10 + (fen[i] - '0'); }

	_state.emplace(board, isBlack, castling, enTarget, hClock, fClock, wKingSquare, bKingSquare);
}

/**
 * TODO: Due to the rest of the infastructure for the player expecting a single, globally accessible move list,
 * TODO: I'll have to cache a MoveTable for use on Non-AI turns, so we can handle lighting things up.
 * (pretty sure I did this already)
 * 
 * TODO: After this, we'll want to rethink our approach to Negamax. For TicTacToe it made sense to store everything
 * TODO: in a TicTacToeAI struct, but Chess is too complicated for that... I'll have to rewatch some of Graeme's
 * TODO: lectures, or ask for help in the Discord.
 * (I did this but broken a bit)
 * 
 * TODO: Lastly, don't forget about the move generation errors that still persist. The King still thinks he's in check
 * TODO: if the knight is in the same row as him. I don't think it'll be hard to track this bug down, but this must be fixed.
 * TODO: thankfully, the test cases we have lying around should greatly speedup testing & bug fixing.
 * 
 * When doing threading, a thread-safe transposition table is neccesary
 * 
 * threefoldRepetitionList
 * 
 * Filter Illegal Moves
 */


// ========================== Misc AI ==========================

const int inf = 999999UL;

// this is the function that will be called by the AI
void Chess::updateAI() {
	// run negamax on all avalible positions.
	int bestVal = -inf;
	Move* bestMove = nullptr;

	MoveTable firstMoves = MoveGenerator(currState, false);

	for(int fromSquare = 0; fromSquare < 64; fromSquare++) {
		std::vector<Move>& moveList = firstMoves.at(fromSquare);
		for (int i = 0; i < moveList.size(); i++) {
			ChessAI newState = ChessAI(GameState(currState, moveList[i]));
			int moveVal = -newState.negamax(newState, 0, -inf, inf, currState.isBlackTurn());
			
			// Logger::getInstance().log(std::to_string(i) + " eval is " + std::to_string(moveVal));

			if (moveVal > bestVal) {
				bestMove = &moveList[i];
				bestVal  = moveVal;
			}
		}
	}

	if (bestMove) {
		_state.emplace(currState, *bestMove);
	}
}

bool Chess::draw(const MoveTable& moves, bool inCheck) {
	if (!inCheck && moves.empty()) {
		return true;
	}

	return false;
}