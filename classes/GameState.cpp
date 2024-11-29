#include <cstring>

#include "GameState.h"
#include "../tools/Logger.h"
#include "magicbitboards.h"

const uint8_t MAX_DEPTH = 24; // for AI purposes
// Generate from FEN
GameState::GameState(const ProtoBoard& board, const bool isBlack, const uint8_t castling,
	const uint8_t enTarget, const uint8_t hClock, const uint16_t fClock,
	const uint8_t wKingSquare, const uint8_t bKingSquare) 
	: bits(board), 
	isBlack(isBlack),
	castlingRights(castling),
	enPassantSquare(enTarget),
	halfClock(hClock),
	clock(fClock),
	friendlyKingSquare(isBlack ? bKingSquare : wKingSquare),
	enemyKingSquare(isBlack ? wKingSquare : bKingSquare) { }

// generate next move
GameState::GameState(const GameState& old, const Move& move)
	: bits(old.bits),
	isBlack(!old.isBlack),
	// scuffed bit funkery: take old rights, mask out side's bits if castling happened.
	castlingRights(old.castlingRights & ~((move.KingSideCastle()  ? (isBlack ? 0b1000 : 0b0010) : 0) 
	                                    | (move.QueenSideCastle() ? (isBlack ? 0b0100 : 0b0001) : 0))),
	// if double push, then get the square jumped over in turn and mark as EnPassant square.
	enPassantSquare(move.isDoublePush() ? (move.getTo() + (isBlack ? -8 : 8)) : 255), // 255 b/c not a value normally reachable in gameplay
	halfClock(move.isCapture() ? 0 : old.halfClock + 1),
	clock(old.clock + 1),
	enemyKingSquare(old.friendlyKingSquare) {

	// don't have to guard against 0 here b/c move.getFrom() will always be a piece.
	ChessPiece piece = bits.PieceFromIndex(move.getFrom());
	bits.enable(piece, move.getTo());
	bits.disable(piece, move.getFrom());

	//std::memcpy(state, old.state, sizeof(state));
	//char movingPiece = state[move.getFrom()];
	//state[move.getTo()] = movingPiece;
	//state[move.getFrom()] = '0';

	if ((piece & 7) == ChessPiece::King) {
		friendlyKingSquare = move.getTo();
	}

	if (move.getTo() == old.enPassantSquare) { // did en passant happen
		uint8_t offset = piece & 8 ? (move.getTo() + 8) : move.getTo() - 8;
		bits.disable((ChessPiece)(piece ^ (1U << 3)), offset);
		//state[movingPiece == 'P' ? (move.getTo() - 8) : (move.getTo() + 8)] = '0';
	} else if (move.isCapture()) {
		ChessPiece target = old.bits.PieceFromIndex(move.getTo());
		bits.disable(target, move.getTo());
	} else if (move.isCastle()) {
		uint8_t offset = isBlack ? 56 : 0;
		uint8_t rookSpot = move.QueenSideCastle() ? 0 : 7 + offset;
		uint8_t targ = (move.QueenSideCastle() ? 3 : 5) + offset;

		ChessPiece lookup = (ChessPiece)(ChessPiece::Rook | (isBlack << 3));

		//state[targ] = state[rookSpot];
		//state[rookSpot] = '0';
		bits.enable(lookup, rookSpot);
		bits.disable(lookup, rookSpot);
	} else if (move.isPromotion()) {
		ChessPiece newPiece;
		switch(move.getFlags() & Move::FlagCodes::Promotion) {
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
		bits.enable((ChessPiece)(newPiece | (isBlack << 3)), move.getTo());
		//newPiece += isBlack ? 32 : 0;
		//state[move.getTo()] = newPiece;
	}
}

GameState::GameState(const GameState& other) {
	castlingRights = other.castlingRights;
	enPassantSquare = other.enPassantSquare;
	halfClock = other.halfClock;
	clock = other.clock;
	isBlack = other.isBlack;
	bits = other.bits;
}

GameState& GameState::operator=(const GameState& other) {
	if (this != &other) {
		castlingRights = other.castlingRights;
		enPassantSquare = other.enPassantSquare;
		halfClock = other.halfClock;
		clock = other.clock;
		isBlack = other.isBlack;
		bits = other.bits;
		//std::memcpy(state, other.state, sizeof(state));
	}
	return *this;
}

bool GameState::operator==(const GameState& other) {
	if (this == &other) return true;
	return 	castlingRights == other.castlingRights &&
			enPassantSquare == other.enPassantSquare &&
			halfClock == other.halfClock &&
			clock == other.clock &&
			isBlack == other.isBlack;
}

uint64_t GameState::getSlidingAttackBoard() const {
	uint64_t attackMap = 0ULL;
	// update sliding attack lanes
	uint64_t occupancy = bits.getOccupancyBoard();
	int offset = (!isBlackTurn() << 3);
	{
		std::vector<uint8_t> rooks = bits.getBitPositions((ChessPiece)(ChessPiece::Rook | offset));
		for (int i : rooks) {
			attackMap |= getRookAttacks(i, occupancy);
			attackMap & ~getFriendlyOccuupancySquare();
			Loggy.log(Logger::WARNING, "Rook - " + std::to_string(i) + " " + std::to_string(getRookAttacks(i, occupancy)));
		}
	}

	{
		std::vector<uint8_t> bishops = bits.getBitPositions((ChessPiece)(ChessPiece::Bishop | offset));
		for (int i : bishops) {
			attackMap |= getBishopAttacks(i, occupancy);
			attackMap & ~getFriendlyOccuupancySquare();
			Loggy.log(Logger::WARNING, "Bish - " + std::to_string(i) + " " + std::to_string(getBishopAttacks(i, occupancy)));
		}
	}

	{
		std::vector<uint8_t> queens = bits.getBitPositions((ChessPiece)(ChessPiece::Queen | offset));
		for (int i : queens) {
			attackMap |= getQueenAttacks(i, occupancy);
			attackMap & ~getFriendlyOccuupancySquare();
			Loggy.log(Logger::WARNING, "Queen - " + std::to_string(i) + " " + std::to_string(getQueenAttacks(i, occupancy)));
		}
	}

	return attackMap;
}