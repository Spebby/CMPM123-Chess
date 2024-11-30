#include <cmath>
#include <stdexcept>
#include "Chess.h"
#include "magicbitboards.h"
#include "../tools/Logger.h"

#define currState _state.top()

Chess::Chess() {
	// precompute dist.
	// may be possible to do this fancier w/ constexpr
	for (int file = 0; file < 8; file++) {
		for (int rank = 0; rank < 8; rank++) {
			int north = 7 - rank;
			int south = rank;
			int west  = file;
			int east  = 7 - file;

			int i = rank * 8 + file;
			_dist[i][0] = north;
			_dist[i][1] = east;
			_dist[i][2] = south;
			_dist[i][3] = west;
			_dist[i][4] = std::min(north, east);
			_dist[i][5] = std::min(south, east),
			_dist[i][6] = std::min(south, west),
			_dist[i][7] = std::min(north, west);
		}
	}
	
	initMagicBitboards();

	// TODO: replace _state with vector & manage it like it's a stack internally.
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
	for (unsigned int k = 0; k < _moves[i].size(); k++) {
		if (_moves[i][k].getTo() == j) {
			return &_moves[i][k];
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
			_grid[file * 8 + rank].initHolder((ImVec2(rank * 64 + 50, (_gameOps.Y - file) * 64 + 50)),
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
	startGame();
	MoveGenerator(false);
}

// I'll probably have to move these into move generator so I can recursively call move generator
bool inCheck;
bool pinned;
bool doubleCheck;
bool pinInPosition;
uint8_t checkRayBitmask;
uint8_t pinRayBitmask;
uint8_t friendlyKingSquare;
uint64_t attackMap;
const int dir[8] = {8, 1, -8, -1, 9, -7, -9, 7};
bool generateQuiets;


inline void ReInitGen() {
	inCheck = false;
	pinned = false;
	doubleCheck = false;
	pinInPosition = false;
	checkRayBitmask = 0U;
	pinRayBitmask = 0U;
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

    // Loop through all directions and find the match
    for (int i = 0; i < 8; ++i) {
        // Determine row and column change for each direction
        int d_row = (dir[i] == 8 || dir[i] == -8 || dir[i] == 9 || dir[i] == -9) ? (dir[i] > 0 ? 1 : -1) : 0;
        int d_col = (dir[i] == 1 || dir[i] == -1 || dir[i] == 9 || dir[i] == -9) ? (dir[i] > 0 ? 1 : -1) : 0;

        // Compare delta_row and delta_col with the direction's row and column changes
        if (delta_row == d_row && delta_col == d_col) {
            return i; // Return the matching direction
        }
    }

    return 0; // Return 0 if no valid direction is found
}

void Chess::CalculateAttackData() {
	attackMap = 0ULL;
	bool enemy = !currState.isBlackTurn();
	// update sliding attack lanes
	uint64_t occupancy = currState.getOccupancyBoard();
	
	{
		uint64_t rooks = currState.getPieceOccupancyBoard(ChessPiece::Rook, enemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getRookAttacks(square, occupancy);
		}, rooks);

		uint64_t bishops = currState.getPieceOccupancyBoard(ChessPiece::Bishop, enemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getBishopAttacks(square, occupancy);
		}, bishops);

		uint64_t queens = currState.getPieceOccupancyBoard(ChessPiece::Queen, enemy);
		forEachBit([&](uint8_t square) {
			attackMap |= getQueenAttacks(square, occupancy);
		}, queens);
	}

	// check around king for pins
	uint64_t kingAdjacent = KingAttacks[friendlyKingSquare];
	ChessBit* kingBit = _grid[friendlyKingSquare].bit();

	forEachBit([&](uint8_t square) {
		int direction = getDirectionOffset(friendlyKingSquare, square);
		int n = _dist[friendlyKingSquare][direction];
		int dirOffset = dir[direction];
		bool friendlyBlock = false;
		uint64_t rayMask = 0;
		bool onDiagonal = direction > 3;

		for (int i = 0; i < n; i++) {
			uint8_t cur = friendlyKingSquare + dirOffset * (i + 1);
			rayMask |= 1ULL << cur;

			ChessBit* piece = _grid[cur].bit();

			if (!piece) continue;

			if (piece->isAlly(kingBit)) {
				if (!friendlyBlock) {
					friendlyBlock = true;
				} else {
					break;
				}

				continue;
			}

			// Enemy piece here
			ChessPiece pieceType = (ChessPiece)(piece->gameTag() & 7);
			if ((onDiagonal && IsDiagonalPiece(pieceType)) || (!onDiagonal && IsHorizontalPiece(pieceType))) {
				// Friendly piece blocks, so it is pinned
				if (friendlyBlock) {
					pinInPosition = true;
					pinRayBitmask |= rayMask;
				} else {
					// no block, so we're in check.
					checkRayBitmask |= rayMask;
					// Are we in double check?
					doubleCheck = inCheck;
					inCheck = true;
				}
				break;
			} else {
				break;
				// piece cannot attack king, blocking any potential pins/checks
			}
		}

		// we don't need to search any more if we're already in doubleCheck.
		if (doubleCheck) {
			return;
		}

	// Since attackMap currently only has sliding data, any piece adjacent to the king that's in danger
	// is likely to threaten the king. So we filter out rays that are obviously safe.
	}, (kingAdjacent & attackMap));

	uint8_t enemyKingSquare = currState.getEnemyKingSquare();

	// king attacks
	attackMap |= KingAttacks[enemyKingSquare];

	// Knight Attacks
	uint64_t knights = currState.getPieceOccupancyBoard(ChessPiece::Knight, enemy);
	bool isKnightCheck = false;
	forEachBit([&](uint8_t fromSquare) {
		uint64_t attacks = KnightAttacks[fromSquare];
		attackMap |= attacks;
		// if knight is attacking king, update doubleCheck status.
		if (!isKnightCheck && ((attacks & (1ULL << enemyKingSquare)) != 0)) {
			isKnightCheck = true;
			doubleCheck = inCheck;
			inCheck = true;
			checkRayBitmask |= 1ULL << fromSquare;
		}
	}, knights);

	// Pawn Attacks
	// TODO: pawn bitboards :(
	/*
	uint64_t pawns = currState.getPieceOccupancyBoard(ChessPiece::Pawn, enemy);
	bool isPawnCheck = false;
	forEachBit([&](uint8_t fromSquare) {
		// pawn attacks
		;
	}, pawns);*/

	// TODO: add check to attack map for potential checks after enpassant

	Loggy.log(Logger::WARNING, std::to_string(attackMap));
}

void Chess::MoveGenerator(bool capturesOnly) {
	// this isn't optimised the best; in the future we'll want to use bitboards instead.
	_moves.clear();

	generateQuiets = !capturesOnly;

	// reset variables.
	ReInitGen();
	friendlyKingSquare = currState.getFriendlyKingSquare();
	CalculateAttackData();

	GenerateKingMoves();

	if (doubleCheck) {
		return;
	}

	GeneratePawnMoves();
	GenerateKnightMoves();

	GenerateBishopMoves();
	GenerateRookMoves();
	GenerateQueenMoves();
}

void Chess::GeneratePawnMoves() {
	bool black = currState.isBlackTurn();
	uint64_t pawns = currState.getPieceOccupancyBoard(ChessPiece::Pawn, black);
	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		int moveDir = black ? dir[2] : dir[0];
		bool canDPush = black ? ((fromSquare / 8) == 6) : ((fromSquare / 8) == 1);

		uint8_t toSquare = fromSquare + moveDir;
		bool canPromote = toSquare == (black ? (fromSquare % 8) : (fromSquare % 8) + 56);
		if (((friends & (1ULL << fromSquare)) != 0)) {
			if (canPromote) {
				for (int i = 0; i < 4; i++) {
					_moves[fromSquare].emplace_back(fromSquare, toSquare, Move::FlagCodes::ToQueen << i);
				}
			} else {
				_moves[fromSquare].emplace_back(fromSquare, toSquare);
				toSquare += moveDir;
				if (canDPush && ((friends & (1ULL << fromSquare)) != 0)) {
					// TODO: double check this code, b/c I am VERY TIRED as I'm writing this and there is 100% an oversight here.
					_moves[fromSquare].emplace_back(fromSquare, toSquare, Move::FlagCodes::DoublePush);
				}
			}
		}

		toSquare = fromSquare + moveDir;
		for (int dirIndex : {3, 1}) {
			const int nTarg = toSquare + dir[dirIndex];
			// First case:  is nTarg to the left of targ?
			// Second case: is nTarg to the right of targ?
			bool lValid = (dirIndex == 3) && (nTarg % _gameOps.X) < (toSquare  % _gameOps.X);
			bool rValid = (dirIndex == 1) && (toSquare  % _gameOps.X) < (nTarg % _gameOps.X);
			if (lValid || rValid) {
				bool enPassant = currState.getEnPassantSquare() == nTarg;
				bool capture   = ((1ULL << toSquare) & enemies) != 0;
				// if enpassant square is specified, then we know it's a legal move b/c en passant square is set on previous turn.
				if (enPassant || capture) {
					_moves[fromSquare].emplace_back(fromSquare, nTarg, Move::FlagCodes::Capture);
				}
			}
		}
	}, pawns);
}

void Chess::GenerateKnightMoves() {
	uint64_t knights = currState.getPieceOccupancyBoard(ChessPiece::Knight, currState.isBlackTurn());
	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare));

		uint64_t attacks = KnightAttacks[fromSquare];
		attacks &= ~friends;
		_moves[fromSquare].reserve(popCount(attacks));
		forEachBit([&](uint8_t toSquare) {
			bool capture = ((1ULL << toSquare) & enemies) != 0;
			// if generating only captures, or this is not a blocking move while in check, break.
			if ((inCheck && !squareIsInCheckRay(toSquare))) return;
			// skip if in check & knight is not going to block/capture attacking piece
			uint16_t flags = capture ? Move::FlagCodes::Capture : 0;
			_moves[fromSquare].emplace_back(fromSquare, toSquare, flags);
		}, attacks);
	}, knights);
}

void Chess::GenerateBishopMoves() {
	uint64_t bishops = currState.getPieceOccupancyBoard(ChessPiece::Bishop, currState.isBlackTurn());
	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare));
		uint64_t attacks = getBishopAttacks(fromSquare, occupancy);
		attacks &= ~friends;
		_moves[fromSquare].reserve(popCount(attacks));

		forEachBit([&](uint8_t toSquare) {
			uint16_t flags = ((1ULL << toSquare) & enemies) ? Move::FlagCodes::Capture : 0;
			_moves[fromSquare].emplace_back(fromSquare, toSquare, flags);
		}, attacks);
	}, bishops);
}

void Chess::GenerateRookMoves() {
	// may be good to handle this in GameState rather than here or do a helper.
	uint64_t rooks = currState.getPieceOccupancyBoard(ChessPiece::Rook, currState.isBlackTurn());
	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare));

		uint16_t flags = 0;
		if        (fromSquare == 0 || fromSquare == 56) {
			flags |= Move::FlagCodes::QCastle;
		} else if (fromSquare == 7 || fromSquare == 63) {
			flags |= Move::FlagCodes::KCastle;
		}

		uint64_t attacks = getRookAttacks(fromSquare, occupancy);
		attacks &= ~friends;
		_moves[fromSquare].reserve(popCount(attacks));

		forEachBit([&](uint8_t toSquare) {
			flags |= ((1ULL << toSquare) & enemies) ? Move::FlagCodes::Capture : 0;
			_moves[fromSquare].emplace_back(fromSquare, toSquare, flags);
		}, attacks);
	}, rooks);
}

void Chess::GenerateQueenMoves() {
		// may be good to handle this in GameState rather than here or do a helper.
	uint64_t queens = currState.getPieceOccupancyBoard(ChessPiece::Queen, currState.isBlackTurn());
	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	forEachBit([&](uint8_t fromSquare) {
		if (inCheck && isPinned(fromSquare));
		uint64_t attacks = getQueenAttacks(fromSquare, occupancy);
		attacks &= ~friends;
		_moves[fromSquare].reserve(popCount(attacks));

		forEachBit([&](uint8_t toSquare) {
			uint16_t flags = ((1ULL << toSquare) & enemies) ? Move::FlagCodes::Capture : 0;
			_moves[fromSquare].emplace_back(fromSquare, toSquare, flags);
		}, attacks);
	}, queens);
}

void Chess::GenerateKingMoves() {
	bool black = currState.isBlackTurn();
	ChessSquare square = _grid[friendlyKingSquare];

	const uint64_t occupancy = currState.getOccupancyBoard();
	const uint64_t enemies   = currState.getEnemyOccuupancyBoard();
	const uint64_t friends   = currState.getFriendlyOccuupancyBoard();

	uint8_t rights = currState.getCastlingRights();

	uint64_t map = KingAttacks[friendlyKingSquare];
	forEachBit([&](uint8_t toSquare) {
		// skip friendly squares
		if (friends & (1ULL << toSquare)) return;

		bool capture = (enemies & (1ULL << toSquare));
		uint16_t flags = capture ? Move::FlagCodes::Capture : 0;
		flags |= Move::FlagCodes::Castling;

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
			_moves[friendlyKingSquare].emplace_back(friendlyKingSquare, toSquare, flags);

			// castling
			if (!inCheck && !capture) {
				// KingSide, f1, f8
				if (((toSquare == 5) || (toSquare == 61)) && ((rights & black ? 0b0100 : 0b0001) != 0)) {
					uint64_t mask = 0b01100000;
					uint8_t castleSquare = toSquare + 1;
					if (((attackMap & mask) != 0) && ((occupancy & mask) != 0)) {
						_moves[friendlyKingSquare].emplace_back(friendlyKingSquare, castleSquare, flags);
					}
				// QueenSide, d1, d8
				} else if (((toSquare == 3) || (toSquare == 59)) && ((rights & black ? 0b1000 : 0b0010) != 0)) {
					uint64_t mask = 0b0111;
					uint8_t castleSquare = toSquare - 1;
					if (((attackMap & mask) != 0) && ((occupancy & mask) != 0)) {
						_moves[friendlyKingSquare].emplace_back(friendlyKingSquare, castleSquare, flags);
					}
				}
			}
		}

	}, map);
}

bool Chess::isPinned(int index) {
	return pinInPosition && ((pinRayBitmask >> index) & 1) != 0;
}

bool Chess::isMovingAlongRay(int asile, int startSquare, int targetSquare) {
	int moveDir = dir[targetSquare - startSquare + 63];
	return (asile == moveDir || -asile == moveDir);
}

// Helper for checking 
bool Chess::squareIsInCheckRay(int square) {
	return inCheck && ((checkRayBitmask >> square) & 1) != 0;
}

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

	if (_moves.count(i)) {
		canMove = true;
		for (Move move : _moves[i]) {
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
	for (Move move : _moves[i]) {
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
	if (j == 63 || j == 56 || j == 0 || j == 7) {
		uint8_t flag = currState.getCastlingRights();
		if (j == 56 || j == 0) {
			flag &= currState.isBlackTurn() ? ~0b0001 : ~0b0100;
		} else if (j == 63 || j == 7) {
			flag &= currState.isBlackTurn() ? ~0b0010 : ~0b1000;
		}
		currState.setCastlingRights(flag);
	}

	// do some check to prompt the UI to select a promotion.

	// call base.
	Game::bitMovedFromTo(bit, src, dst);

	clearPositionHighlights();
	MoveGenerator();
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

	s += std::to_string((int)currState.getHalfClock()) + ' ' + std::to_string((int)currState.getClock());

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

// this is the function that will be called by the AI
void Chess::updateAI() {

}