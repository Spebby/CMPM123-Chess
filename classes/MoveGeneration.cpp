#include "Chess.h"

#include "MagicBitboards/MagicBitboards.h"
#include "MagicBitboards/BitFunctions.h"

#include "PrecomputedData.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

// TODO: Change MoveTable to be a vector.

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

bool Chess::InCheck() {
	return inCheck;
}

std::vector<Move> Chess::MoveGenerator(GameState& state, bool capturesOnly) {
	// this isn't optimised the best; in the future we'll want to use bitboards instead.
	generateQuiets = !capturesOnly;

	std::vector<Move> list;
	// reset variables.
	ReInitGen();
	friendlyKingSquare = state.getFriendlyKingSquare();
	CalculateAttackData(state);

#ifdef DEBUG
	//Loggy.log("Attack Map - " + std::to_string(attackMap));
#endif

	GenerateKingMoves(list, state);

	if (doubleCheck) {
		return list;
	}

	GenerateSlidingMoves(list, state);
	GenerateKnightMoves(list, state);
	GeneratePawnMoves(list, state);

	return list;
}

/*
// thanks for the starter, graeme
void Chess::sortMovesByMVVLVA(ProtoBoard& board, std::vector<Move>& captureMoves) {
    if (captureMoves.size() < 2) return;

    // Implement sorting of moves based on the MVV-LVA heuristic
    // https://www.chessprogramming.org/MVV-LVA
    // https://www.chessprogramming.org/Move_Ordering
    std::sort(captureMoves.begin(), captureMoves.end(), [&](const Move& a, const Move& b) {
					evaluateScores();
                    int victimValueA = Evaluate::getPieceValue(state[a.to]);
                    int attackerValueA = Evaluate::getPieceValue(state[a.from]);
                    int victimValueB = Evaluate::getPieceValue(state[b.to]);
                    int attackerValueB = Evaluate::getPieceValue(state[b.from]);
                    return (victimValueA - attackerValueA) > (victimValueB - attackerValueB); });
}
*/

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
	uint64_t sliderBoard;
	// index of King Square on bitboard.
	const uint64_t kingBit = 1ULL << friendlyKingSquare;

	{
		uint64_t blockers = occupancy ^ kingBit;
		uint64_t rooks = state.getPieceOccupancyBoard(ChessPiece::Rook, blackIsEnemy);
		sliderBoard |= rooks;
		forEachBit([&](uint8_t square) {
			attackMap |= getRookAttacks(square, blockers);
		}, rooks);

		uint64_t bishops = state.getPieceOccupancyBoard(ChessPiece::Bishop, blackIsEnemy);
		sliderBoard |= bishops;
		forEachBit([&](uint8_t square) {
			attackMap |= getBishopAttacks(square, blockers);
		}, bishops);

		uint64_t queens = state.getPieceOccupancyBoard(ChessPiece::Queen, blackIsEnemy);
		sliderBoard |= queens;
		forEachBit([&](uint8_t square) {
			attackMap |= getQueenAttacks(square, blockers);
		}, queens);
	}
#ifdef DEBUG
	//Loggy.log(Logger::WARNING, "Sliding Step - " + std::to_string(attackMap));
#endif

	// we keep track of sliderBoards b/c bitboards don't include the piece's starting square
	// keeping track of this is relatively cheap & most of the time still saves time over just brute force
	// checking every direction.
	uint64_t kingAdjacentDanger = KingAttacks[friendlyKingSquare] & (attackMap | sliderBoard);
	uint64_t friendly = state.getFriendlyOccuupancyBoard();

	// check around king for pins
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
			uint8_t currSquare = friendlyKingSquare + (dirOffset * (i + 1));
			rayMask |= 1ULL << currSquare;

			ChessPiece piece = state.PieceFromIndex(currSquare);
			if (piece == NoPiece) continue;

			bool friendlyPiece = (friendly & (1ULL << currSquare)) != 0;
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

	// Knight Attacks
	uint64_t knights = state.getPieceOccupancyBoard(ChessPiece::Knight, blackIsEnemy);
	bool isKnightCheck = false;
	uint64_t knightAttackMap = 0;
	forEachBit([&](uint8_t fromSquare) {
		uint64_t attacks = KnightAttacks[fromSquare];
		knightAttackMap |= attacks;
		// if knight is attacking king, update doubleCheck status.
		if (!isKnightCheck && ((knightAttackMap & kingBit) != 0)) {
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

		if (!pawnCheck && (pawnAttackMap & kingBit) != 0) {
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

#ifdef DEBUG
	//Loggy.log(Logger::WARNING, "Pawn Step - " + std::to_string(attackMap));
	//Loggy.log(Logger::WARNING, "Check Ray - " + std::to_string(checkRayBitmask));
	//Loggy.log(Logger::WARNING, "Pin Ray   - " + std::to_string(pinRayBitmask));
#endif
}

void Chess::GeneratePawnMoves(std::vector<Move>& moves, GameState& state) {
	uint64_t pawns = state.getPieceOccupancyBoard(ChessPiece::Pawn, state.isBlackTurn());
	const uint64_t occupancy = state.getOccupancyBoard();
	const uint64_t enemies   = state.getEnemyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		GeneratePawnPush(moves, state, occupancy, fromSquare);
		GeneratePawnAttack(moves, state, enemies, fromSquare);
	}, pawns);
}

inline void Chess::GeneratePawnPush(std::vector<Move>& moves, GameState& state, const uint64_t occupancy, const uint8_t fromSquare) {
	int moveDir = state.isBlackTurn() ? dir[2] : dir[0];
	uint8_t toSquare = fromSquare + moveDir;
	
	if ((occupancy & (1ULL << toSquare)) == 0) {
		// Make sure that we continue to block if pushing. 
		if (inCheck && !squareIsInCheckRay(toSquare)) return;

		bool canPromote = toSquare == (state.isBlackTurn() ? (fromSquare % 8) : (fromSquare % 8) + 56);
		if (canPromote) {
			for (int i = 0; i < 4; i++) {
				moves.emplace_back(fromSquare, toSquare, Move::FlagCodes::ToQueen << i);
			}
		} else {
			moves.emplace_back(fromSquare, toSquare);
			bool canDPush = state.isBlackTurn() ? ((fromSquare / 8) == BlackPawnStartRank) : ((fromSquare / 8) == WhitePawnStartRank);
			if (!canDPush) return;

			toSquare += moveDir;
			// there is for sure a cleaner way of doing this, but for the moment we run this check twice so
			// we don't get a situation where we can make this move, even though it doesn't block check.
			if (inCheck && !squareIsInCheckRay(toSquare)) return;
			if (canDPush && ((occupancy & (1ULL << toSquare)) == 0)) {
				moves.emplace_back(fromSquare, toSquare, Move::FlagCodes::DoublePush);
			}
		}
	}
}

inline void Chess::GeneratePawnAttack(std::vector<Move>& moves, GameState& state, const uint64_t enemies, const uint8_t fromSquare) {
	uint64_t attacks = PawnAttacks[fromSquare][state.isBlackTurn()] & enemies;
	//& checkRayBitmask;
	forEachBit([&](uint8_t toSquare) {
		if (inCheck && !squareIsInCheckRay(toSquare)) return;

		bool EnPassant = state.getEnPassantSquare() == toSquare;
		// if enpassant square is specified, then we know it's a legal move b/c en passant square is set on previous turn.

		// promote captures combos
		bool canPromote = toSquare == (state.isBlackTurn() ? (toSquare % 8) : (toSquare % 8) + 56);
		if (canPromote) {
			for (int i = 0; i < 4; i++) {
				moves.emplace_back(fromSquare, toSquare, Move::FlagCodes::ToQueen << i);
			}
		} else {
			moves.emplace_back(fromSquare, toSquare, EnPassant ? Move::FlagCodes::EnCapture : 0);
		}
	}, attacks);
}

void Chess::GenerateKnightMoves(std::vector<Move>& moves, GameState& state) {
	// all non-pinned knights
	uint64_t knights = state.getPieceOccupancyBoard(ChessPiece::Knight, state.isBlackTurn()) & ~pinRayBitmask;
	const uint64_t friends   = state.getFriendlyOccuupancyBoard();
	const uint64_t moveMask  = ~friends;
	//& checkRayBitmask;

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare)) return;

		uint64_t attacks = KnightAttacks[fromSquare] & moveMask;
		moves.reserve(popCount(attacks));
		forEachBit([&](uint8_t toSquare) {
			// if generating only captures, or this is not a blocking move while in check, break.
			if ((inCheck && !squareIsInCheckRay(toSquare))) return;
			// skip if in check & knight is not going to block/capture attacking piece
			moves.emplace_back(fromSquare, toSquare);
		}, attacks);
	}, knights);
}

#include <functional>

// TODO: consider merging sliding moves down to simplify things. Queen doesn't need her own step.
void Chess::GenerateSlidingMoves(std::vector<Move>& moves, GameState& state) {
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

void Chess::GenerateSlidingMovesHelper(std::vector<Move>& moves, const std::function<uint64_t(uint8_t, uint64_t)>& getAttacksFunc, const uint64_t& pieceMap, const uint64_t& occupancy, const uint64_t& enemies, const uint64_t& moveMask) {
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
			moves.emplace_back(fromSquare, toSquare);
		}, attacks);
	}, pieceMap);
}

void Chess::GenerateKingMoves(std::vector<Move>& moves, GameState& state) {
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
			moves.emplace_back(friendlyKingSquare, toSquare, flags);
		}
	}, attacks);

	bool canCastleKingSide  = (state.getCastlingRights() & (black ? 0b0010 : 0b1000)) != 0;
	bool canCastleQueenSide = (state.getCastlingRights() & (black ? 0b0001 : 0b0100)) != 0;

	// castling
	if (!inCheck && generateQuiets) {
		const uint64_t occupancy = state.getOccupancyBoard();
		// KingSide, f1, f8
		const uint64_t blockers = attackMap | occupancy;
		if (canCastleKingSide) {
			const uint64_t mask = state.isBlackTurn() ? PositionMasks::BlackKingsideMask : PositionMasks::WhiteKingsideMask;
			if ((mask & blockers) == 0) {
				const uint8_t castleSquare = friendlyKingSquare + 1;
				moves.emplace_back(friendlyKingSquare, castleSquare, Move::FlagCodes::Castling);
			}
		// QueenSide, d1, d8
		} else if (canCastleQueenSide) {
			const uint64_t mask      = state.isBlackTurn() ? PositionMasks::BlackQueensideMask      : PositionMasks::WhiteQueensideMask;
			const uint64_t blockMask = state.isBlackTurn() ? PositionMasks::BlackQueensideBlockMask : PositionMasks::WhiteQueensideBlockMask;
			if (((mask & blockers) == 0) && ((blockMask & occupancy) == 0)) {
				const uint8_t castleSquare = friendlyKingSquare - 2;
				moves.emplace_back(friendlyKingSquare, castleSquare, Move::FlagCodes::Castling);
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

// TODO: with clever use of checkRayBitmask this funciton is entierly uneeded.
bool Chess::squareIsInCheckRay(int square) {
	return inCheck && ((checkRayBitmask >> square) & 1) != 0;
}