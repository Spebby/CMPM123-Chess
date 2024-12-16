#include <cstring>
#include "GameState.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

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
	enemyKingSquare(isBlack ? wKingSquare : bKingSquare),
	capturedPieceType(NoPiece) {}

// generate next move
GameState::GameState(const GameState& old, const Move& move)
	: bits(old.bits),
	isBlack(!old.isBlack),
	// scuffed bit funkery: take old rights, mask out side's bits if castling happened.
	castlingRights(old.castlingRights & ~((move.KingSideCastle()  ? (isBlack ? 0b1000 : 0b0010) : 0) 
	                                    | (move.QueenSideCastle() ? (isBlack ? 0b0100 : 0b0001) : 0))),
	// if double push, then get the square jumped over in turn and mark as EnPassant square.
	enPassantSquare(move.isDoublePush() ? (move.getTo() + (isBlack ? -8 : 8)) : 255), // 255 b/c not a value normally reachable in gameplay
	halfClock(old.halfClock + 1),
	clock(old.isBlack ? old.clock + 1 : old.clock),
	friendlyKingSquare(old.enemyKingSquare),
	enemyKingSquare(old.friendlyKingSquare),
	capturedPieceType(NoPiece)
	{

	uint8_t from = move.getFrom();
	uint8_t to   = move.getTo();

	ChessPiece piece  = bits.PieceFromIndex(from);
	ChessPiece target = bits.PieceFromIndex(to);
	bool capture = (target != NoPiece);

	bits.enable(piece, to);
	bits.disable(piece, from);

	// updating enemy king square may seem weird, but keep in mind that a state represents the board as the player
	// is making their move.
	if ((piece & 7) == ChessPiece::King) {
		enemyKingSquare = to;
		castlingRights &= !isBlack ? ~0b0011 : ~0b1100;

		if (move.isCastle()) {
			const uint8_t offset = !isBlack ? 56 : 0;
			const uint8_t originalRookSquare = (move.QueenSideCastle() ? 0 : 7) + offset;
			const uint8_t movedRookSquare    = (move.QueenSideCastle() ? 3 : 5) + offset;

			const ChessPiece lookup = (ChessPiece)(ChessPiece::Rook | (!isBlack << 3));

			bits.enable(lookup, movedRookSquare);
			bits.disable(lookup, originalRookSquare);
		}
	} else if ((piece & 7) == ChessPiece::Pawn) {
		halfClock = 0;
	}

	if (to == old.enPassantSquare) {
		halfClock = 0;
		uint8_t offset = piece & 8 ? (to + 8) : to - 8;
		bits.disable((ChessPiece)(piece ^ (1U << 3)), offset);
		capturedPieceType = Pawn;
	} else if (capture) {
		halfClock = 0;
		capturedPieceType = target;
		bits.disable(target, to);

		// update castling rights if a rook was taken
		if (to == 56 || to == 0) {
			castlingRights &= isBlackTurn() ? ~0b0001 : ~0b0100;
		} else if (to == 63 || to == 7) {
			castlingRights &= isBlackTurn() ? ~0b0010 : ~0b1000;
		}
	
	// TODO: at some point i was a dumbass and never put checks here to make sure king couldn't trigger castling
	// by simply moving his ass to the destination square, which summons a rook for him???
	// regardless... make sure to properly put checks here. Also consider changing when move.isCastle() can be true
	// perhaps it would make sense to no longer mark every single king move as a castle (duh)
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
		bits.disable((ChessPiece)(Pawn | (!isBlack << 3)), to);
		bits.enable((ChessPiece)(newPiece | (!isBlack << 3)), to);
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

void GameState::MakeMove(const Move& move) {
	isBlack = !isBlack;
	castlingRights = castlingRights & ~((move.KingSideCastle()  ? (isBlack ? 0b1000 : 0b0010) : 0) | (move.QueenSideCastle() ? (isBlack ? 0b0100 : 0b0001) : 0));
	halfClock++;
	!isBlack ? clock++ : 0;
	capturedPieceType = NoPiece;

	uint8_t from = move.getFrom();
	uint8_t to   = move.getTo();
	
	ChessPiece piece = bits.PieceFromIndex(from);
	ChessPiece target = bits.PieceFromIndex(to);
	bool capture = (target != NoPiece);

	if ((piece & 7) == ChessPiece::King) {
		friendlyKingSquare = to;
		castlingRights &= !isBlack ? ~0b0011 : ~0b1100;

		if (move.isCastle()) {
			const uint8_t offset = !isBlack ? 56 : 0;
			const uint8_t originalRookSquare = (move.QueenSideCastle() ? 0 : 7) + offset;
			const uint8_t movedRookSquare    = (move.QueenSideCastle() ? 3 : 5) + offset;

			const ChessPiece lookup = (ChessPiece)(ChessPiece::Rook | (!isBlack << 3));

			bits.enable(lookup, movedRookSquare);
			bits.disable(lookup, originalRookSquare);
		}
	} else if ((piece & 7) == ChessPiece::Pawn) {
		halfClock = 0;
	}

	// Swapping King pos after King moves so we don't accidentally overwrite the wrong king position.
	uint8_t temp = friendlyKingSquare;
	friendlyKingSquare = enemyKingSquare;
	enemyKingSquare = temp;

	// not updated enPassantSquare yet so still "old" move.
	if (to == enPassantSquare) {
		halfClock = 0;
		uint8_t offset = piece & 8 ? (to + 8) : to - 8;
		bits.disable((ChessPiece)(piece ^ (1U << 3)), offset);
		capturedPieceType = Pawn;
	} else if (capture) {
		halfClock = 0;
		capturedPieceType = target;
		bits.disable(target, to);

		if (to == 56 || to == 0) {
			castlingRights &= isBlackTurn() ? ~0b0001 : ~0b0100;
		} else if (to == 63 || to == 7) {
			castlingRights &= isBlackTurn() ? ~0b0010 : ~0b1000;
		}
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
		bits.disable((ChessPiece)(Pawn | (!isBlack << 3)), to);
		bits.enable((ChessPiece)(newPiece | (!isBlack << 3)), to);
	}

	// We do these at the end now, b/c we can't rely on there being an old board to reference.
	bits.enable(piece, to);
	bits.disable(piece, from);
	enPassantSquare = move.isDoublePush() ? (move.getTo() + (isBlack ? -8 : 8)) : 255;
}

#include <cassert>
void GameState::UnmakeMove(const Move& move, const GameStateMemory& memory) {
	uint8_t temp = friendlyKingSquare;
	friendlyKingSquare = enemyKingSquare;
	enemyKingSquare = temp;

	// for now, we shouldn't worry about undoing fifty move counter.
	isBlack = !isBlack;
	bool undoingBlackMove = isBlack;

	const uint8_t from = move.getFrom();
	const uint8_t to = move.getTo();

	// I'm concerned about undoing & restoring EnPassant for a previous move. Perhaps it would pay to keep previous EnPassant square around?
	bool undoingEnCapture	= move.isEnCapture();
	bool undoingPromotion	= move.isPromotion();
	bool undoingCapture     = (capturedPieceType != NoPiece);

	const ChessPiece mover = bits.PieceFromIndex(to);

	if ((mover & 7) == ChessPiece::King) {
		friendlyKingSquare = from;
		if (move.isCastle()) {
			const uint8_t offset = undoingBlackMove ? 56 : 0;
			const uint8_t originalRookSquare = (move.QueenSideCastle() ? 0 : 7) + offset;
			const uint8_t movedRookSquare    = (move.QueenSideCastle() ? 3 : 5) + offset;

			const ChessPiece rook = undoingBlackMove ? (ChessPiece)(Rook | Black) : Rook;
			bits.enable(rook, originalRookSquare);
			bits.disable(rook, movedRookSquare);
		}
	}

	if (undoingCapture) {
		uint8_t captureSquare = to;
		if (undoingEnCapture) {
			captureSquare = to + ((undoingBlackMove) ? 8 : -8);
			enPassantSquare = captureSquare;
		} else {
			enPassantSquare = 255;
		}

		bits.enable(capturedPieceType, captureSquare);
	}

	bits.disable(mover, to);
	bits.enable(mover, from);
	if (undoingPromotion) {
		bits.enable((ChessPiece)(Pawn | (!undoingBlackMove << 3)), from);
	}

	// Originally was using an internal stack to handle this, but was encountering some very strange bugs w/ popping causing
	// crashes. May be worth investigating that eventually, but this is an acceptable solution for now.

	capturedPieceType = memory.capturedPieceType;
	castlingRights = memory.castlingRights;
	halfClock = memory.halfClock;
	clock -= undoingBlackMove ? 1 : 0;

	//assert(friendlyKingSquare != 11);
}