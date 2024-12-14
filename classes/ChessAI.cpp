#include "ChessAI.h"
#include "MagicBitboards/BitFunctions.h"
#include "MagicBitboards/EvaluationTables.h"

#ifdef DEBUG
#include "../tools/Logger.h"
#endif

const int inf = 999999UL;

ChessAI::ChessAI(GameState state) : _state(state), _board(state.getProtoBoard()) {
    
}

// Piece Values
static std::map<ChessPiece, int> evaluateScores = {
    {Pawn, 100},
    {Knight, 200},
    {Bishop, 230},
    {Rook, 400},
    {Queen, 900},
    {King, 2000}
};

// Returns: positive value if AI wins, negative if human player wins, 0 for draw or undecided
int ChessAI::evaluateBoard() {
	int score = 0;
	for (int i = 0; i < 12; i++) {
		// To simplify my statements a bit, I'll be adding the passes
		// after I calculate their score, so I can subtract if black and add if white.
		ChessPiece piece = ProtoBoard::PieceFromProtoIndex(i);
		bool black = (piece & 8) != 0;
		piece = (ChessPiece)(piece & 7);

		// Add up scores of each piece, ignoring position.
		int passScore = evaluateScores[piece] * popCount(_board[i]);
		forEachBit([&passScore, &piece, &black](uint8_t pos){
			uint8_t truePos = pos;
			if (black) {
				truePos = pos ^ 56;
				// flip
			}

			switch(piece) {
				case Pawn:
					passScore += pawnTable[truePos];
					break;
				case Knight:
					passScore += knightTable[truePos];
					break;
				case Bishop:
					passScore += bishopTable[truePos];
					break;
				case Rook:
					passScore += rookTable[truePos];
					break;
				case Queen:
					passScore += queenTable[truePos];
					break;
			}
		}, _board[i]);

		score += black ? -passScore : passScore;
	}

	return score;
}

/* Graeme's chunked evaluator. For now, ignore this.
inline uint16_t constructIndex(char p1, char p2, char p3, char p4) {
	return (static_cast<uint64_t>(pairToByteTable[p1][p2]) << 8) |
			pairToByteTable[p3][p4];
}

// This is a more advanced evaluation method which evaluates positions based on 2x2 chunks of pieces.
// This way, we can reason more about what formations are advantageous, instead of simply reasoning about
// where an individual piece is.
int Chess::evaluateBoard(const char* state) {
	assert(state != nullptr && "State array cannot be null");

	int score = 0;
	int offTable = 0;
	for (int i = 0; i < 64; i += 4, offTable++) {
		uint16_t index = constructIndex(state[i], state[i + 1], state[i + 2], state[i + 3]);
		score += fourSquareEval[offTable][index];
	}
	return score;
}
*/

int ChessAI::negamax(const int depth, const int distFromRoot, int alpha, int beta) {
    if (depth == 0) {
		// TODO: return quiesce search instead
        return Quiesce(alpha, beta);
    }

	// Todo: TranspositionTable optimisation would be nice.


	MoveTable moves = Chess::MoveGenerator(_state, false);

	/*
	if (moves.size() == 0) {
		if (InCheck()) {
			int score =  - distFromRoot;
			return score;
		}

		return 0;
	}*/

	int bestValue = -inf; // Negative "Infinity"
	//uint64_t bitboard = isBlack ? _board.getWhiteOccupancyBoard() : _board.getBlackOccupancyBoard();

	for (const auto& t : moves) {
		const std::vector<Move> moveList = t.second;
		#ifdef DEBUG
		Loggy.log("Depth: " + std::to_string(depth) + " index: " + std::to_string(t.first));
		#endif
		for (const Move& move : moveList) {
			// Make, Mo' Nega, Unmake, Prune.
			GameStateMemory memory = _state.makeMemoryState();
			_state.MakeMove(move);
			#ifdef DEBUG
			uint64_t bit = logDebugInfo();
			#endif
			int score = -negamax(depth - 1, distFromRoot + 1, -beta, -alpha);
			#ifdef DEBUG
			Loggy.log(std::to_string(score));
			#endif
			_state.UnmakeMove(move, memory);

			if (score > bestValue) {
				bestValue = score;
				if (score > alpha) {
					alpha = score;
				}
			}

			if (score >= beta) {
				return bestValue;
			}
		}
	}

	return bestValue;
}

int ChessAI::Quiesce(int alpha, int beta) {
	return evaluateBoard();
	// TODO

	/*
	int stand_pat = evaluateBoard();
	if (stand_pat >= beta) {
		return beta;
	}
	if (alpha < stand_pat) {
		alpha = stand_pat;
	}

	until (everyCaptureExmained) {
		MakeMove();
		score = -Quiesce(-beta, -alpha);
		UnmakeMove();

		if (score >= beta) {
			return beta;
		}
		if (score > alpha) {
			alpha = score;
		}
	}
	return alpha;
	*/
}

// Eventually add more neuanced draw detection like three fold repetition
bool ChessAI::isDraw() const {
	if (_state.getHalfClock() >= 50) {
		return true;
	}
	
	return false;
};

bool ChessAI::InCheck() const {
	return false;
}

#ifdef DEBUG
uint64_t ChessAI::logDebugInfo() const {
	uint64_t bits = _board.getOccupancyBoard();
	Loggy.log("Occupancy - " + std::to_string(bits) + " - isBlack: " + std::to_string(_state.isBlackTurn()));
	return bits;
}
#endif