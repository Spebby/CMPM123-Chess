#pragma once

enum ChessPiece : uint8_t {
	NoPiece	= 0,
	Pawn	= 1,
	Knight	= 2,
	Bishop	= 3,
	Rook	= 4,
	Queen	= 5,
	King	= 6,
	Black   = 1ULL << 3
};

enum PositionMasks : uint64_t {
	WhiteKingsideMask  = 1ULL << 5  | 1ULL << 6,  // F1, G1
	BlackKingsideMask  = 1ULL << 61 | 1ULL << 62, // F8, G8
	WhiteQueensideMask = 1ULL << 3  | 1ULL << 2,  // C1, D1
	BlackQueensideMask = 1ULL << 59 | 1ULL << 58, // C8, D8
	WhiteQueensideBlockMask = WhiteQueensideMask | 1ULL << 1, // B1, C1, D1
	BlackQueensideBlockMask = BlackQueensideMask | 1ULL << 57, // B8, C8, D8
	WhitePawnStartRank = 1,
	BlackPawnStartRank = 6,
	WhiteKingStartMask = 4,
	BlackKingStartMask = 60
};

inline bool IsDiagonalPiece(ChessPiece piece) {
	return ((piece & 7) == ChessPiece::Queen) || ((piece & 7) == ChessPiece::Bishop);
}

inline bool IsHorizontalPiece(ChessPiece piece) {
	return ((piece & 7) == ChessPiece::Queen) || ((piece & 7) == ChessPiece::Rook);
}

inline bool IsSlidingPiece(ChessPiece piece) {
	return ((piece & 7) == ChessPiece::Queen) ||((piece & 7) == ChessPiece::Rook) || ((piece & 7) == ChessPiece::Bishop);
}