#pragma once

enum ChessPiece {
	NoPiece	= 0,
	Pawn	= 1,
	Knight	= 2,
	Bishop	= 3,
	Rook	= 4,
	Queen	= 5,
	King	= 6
};

inline bool IsDiagonalPiece(ChessPiece piece) {
	return (piece == ChessPiece::Queen) || (piece == ChessPiece::Bishop);
}

inline bool IsHorizontalPiece(ChessPiece piece) {
	return (piece == ChessPiece::Queen) || (piece == ChessPiece::Rook);
}

inline bool IsSlidingPiece(ChessPiece piece) {
	return (piece == ChessPiece::Queen) ||(piece == ChessPiece::Rook) || (piece == ChessPiece::Bishop);
}