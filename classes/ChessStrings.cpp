#include "Chess.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

#define currState _state.top()


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

const uint8_t INVALID_POS = 255;

// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini file and set the game state to the last saved state
// modified from Sebastian Lague's Coding Adventure on Chess. 2:37
void Chess::setStateString(const std::string& fen) {
	ProtoBoard board;
	uint8_t wKingSquare = INVALID_POS;
	uint8_t bKingSquare = INVALID_POS;

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
		int castling = 15;
		// if Kings are not in starting position, then disable that side's ability to castle.

		if (wKingSquare == INVALID_POS || bKingSquare == INVALID_POS) {
			throw std::runtime_error("Invalid FEN string. King is missing!");
		}
		
		if (wKingSquare != WhiteKingStartMask) {
			castling &= 3;
		}
		if (bKingSquare != BlackKingStartMask) {
			castling &= 12;
		}

		_state.emplace(board, 0, castling, INVALID_POS, 0, 0, wKingSquare, bKingSquare);
		return;
	}

	// extract the game state part of FEN
	bool isBlack = (fen[i] == 'b');
	i += 2;

	bool specifiesRights;
	uint8_t castling = 0;
	while (i < fen.size() && fen[i] != ' ') {
		switch (fen[i++]) {
			case 'K': castling |= 1 << 3; specifiesRights = true; break;
			case 'Q': castling |= 1 << 2; specifiesRights = true; break;
			case 'k': castling |= 1 << 1; specifiesRights = true; break;
			case 'q': castling |= 1; specifiesRights = true; break;
			case '-': castling  = 0; specifiesRights = true; break;
		}
	}
	i++;

	if (!specifiesRights) {
		castling = 15;
	}

	// if Kings are not in starting position, then disable that side's ability to castle.
	if (wKingSquare != WhiteKingStartMask) {
		castling &= 3;
	}
	if (bKingSquare != BlackKingStartMask) {
		castling &= 12;
	}

	uint8_t enTarget = INVALID_POS;
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

	// TODO: analyse the fen string to make sure king enemy king is not in check.
	// if it is, throw an error.
}