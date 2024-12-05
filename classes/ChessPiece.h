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

enum BitMasks : uint64_t {
	WhiteKingsideMask  = 1ULL << 5  | 1ULL << 6,  // F1, G1
	BlackKingsideMask  = 1ULL << 61 | 1ULL << 62, // F8, G8
	WhiteQueensideMask = 1ULL << 3  | 1ULL << 2,  // C1, D1
	BlackQueensideMask = 1ULL << 59 | 1ULL << 58, // C8, D8
	WhiteQueensideBlockMask = WhiteQueensideMask | 1ULL << 1, // B1, C1, D1
	BlackQueensideBlockMask = BlackQueensideMask | 1ULL << 57 // B8, C8, D8
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