#pragma once
#include <cstdint>

// I'm ultimately conflicted on what size I should store my moves as.
// Since I'm not aiming to do bitboards for this leg of the project (maybe get to in the future?)
// max efficientcy isn't neccesary, and I'd like to have the wiggle room.

// implementaiton borrowed from w/ modificaitons https://www.chessprogramming.org/Encoding_Moves

#pragma pack(push, 1)
const uint8_t squareMask = 63U;
class Move {
	public:
	enum FlagCodes {
		// I included some flags in here for convenience later.
		EnCapture  = 0b00000001,
		DoublePush = 0b00000010,
		Castling   = 0b00001100,
		QCastle    = 0b00000100,
		KCastle    = 0b00001000,
		Promotion  = 0b11110000,
		ToQueen    = 0b00010000,
		ToKnight   = 0b00100000,
		ToRook     = 0b01000000,
		ToBishop   = 0b10000000,
		// not checking
	};

	Move(uint8_t from, uint8_t to);
	Move(uint8_t from, uint8_t to, uint8_t flags);
	Move(const Move& other) : move(other.move) {}

	Move& operator=(const Move&);
	bool operator==(const Move& a) const { return move == a.move; }
	bool operator!=(const Move& a) const { return move != a.move; }

	uint8_t getTo()	const { return move & squareMask; }
	uint8_t getFrom()  const { return (move >> 6) & squareMask; }
	inline uint8_t getFlags() const { return (move >> 12); }

	void setTo(uint8_t);
	void setFrom(uint8_t);
	// Overwrite the flag completely.
	void setFlags(uint8_t);
	// Set specific bits of the flag, and leave the rest unchanged.
	void setFlagBits(uint8_t);
	// toggles selected bits from the flag mask.
	void toggleFlags(uint8_t);

	// really wish C++ had C# styled lambdas right now

	// NOTE: The convenience of checking if a captured happened was outweighed by unmaking enpassant
	// I can't store both with the current flag size limit, and if enpassant had happened was impossible to
	// determine dynamically, where as a regular capture is.
	bool isEnCapture()		const { return (getFlags() &  FlagCodes::EnCapture)		!= 0; }
	//bool isQuiet()			const { return (getFlags() & ~FlagCodes::Capture)		!= 0; }
	bool isDoublePush()		const { return (getFlags() &  FlagCodes::DoublePush)	!= 0; }
	bool isPromotion()		const { return (getFlags() &  FlagCodes::Promotion)		!= 0; }
	bool KingSideCastle()	const { return (getFlags() &  FlagCodes::KCastle)		!= 0; }
	bool QueenSideCastle()	const { return (getFlags() &  FlagCodes::QCastle)		!= 0; }
	bool isCastle()			const { return (getFlags() &  FlagCodes::Castling)      != 0; }

	// Future proofing
	uint16_t getButterflyIndex() const { return move & 0x0fff; }

	protected:
	// we only use 24 bits, but avoiding bitfield to improve portability.
	uint32_t move;
};
#pragma pack(pop)