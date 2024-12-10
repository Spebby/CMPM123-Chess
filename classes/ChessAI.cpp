#include "ChessAI.h"
#include "MagicBitboards/BitFunctions.h"
#include "MagicBitboards/EvaluationTables.h"

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
    {King, 3000}
};

// Returns: positive value if AI wins, negative if human player wins, 0 for draw or undecided
int ChessAI::evaluateBoard(const ProtoBoard& board) const {
	int score = 0;
	for (int i = 0; i < 12; i++) {
		// To simplify my statements a bit, I'll be adding the passes
		// after I calculate their score, so I can subtract if black and add if white.
		ChessPiece piece = board.PieceFromIndex(i);
		bool black = (piece & 8) != 0;
		piece = (ChessPiece)(piece & 7);

		// i love bitboards
		int passScore = evaluateScores[piece] * popCount(board[i]);
		forEachBit([&passScore, &board, &piece](uint8_t pos){
			switch(piece) {
				case Pawn:
					passScore += pawnTable[pos];
					break;
				case Knight:
					passScore += knightTable[pos];
					break;
				case Bishop:
					passScore += bishopTable[pos];
					break;
				case Rook:
					passScore += rookTable[pos];
					break;
				case Queen:
					passScore += rookTable[pos];
					break;
			}
		}, board[i]);

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

// init like this : negamax(rootState, depth, -inf, +inf, 1)
// player is the current player's number (AI or human)
int ChessAI::negamax(ChessAI& state, const int depth, int alpha, int beta, const bool isBlack) {
	// TODO: move this to GameState
	int score = state.evaluateBoard(_board);

    if (depth == 0 || state._state.getHalfClock() == 50) {
        return score;
    }

	MoveTable moves = Chess::MoveGenerator(state._state, false);
	int bestVal = -inf; // Negative "Infinity"
	uint64_t bitboard = state._state.getFriendlyOccuupancyBoard();
	while(bitboard) {
		uint8_t index = bitScanForward(bitboard);
        bitboard &= bitboard - 1;

		for (const Move& move : moves[index]) {
			// Make, Mo' Nega, Unmake, Prune.
			_state + move;
			bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, !isBlack));
			_state - move;

			alpha = std::max(alpha, bestVal);
			if (alpha >= beta) {
				break;
			}
		}
	};

	return bestVal;
}

// AI class
int ChessAI::AICheckForWinner() const {


	return 0;
}