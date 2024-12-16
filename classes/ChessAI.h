#pragma once

#include "Chess.h"

class ChessAI {
    public:
    ChessAI(GameState);

    // Returns: positive value if AI wins, negative if human player wins, 0 for draw or undecided
    int evaluateBoard();

    // init like this : negamax(rootState, depth, -inf, +inf, 1)
    // player is the current player's number (AI or human)
    int negamax(const int depth, const int distFromRoot, int alpha, int beta, const int player);
    // TODO: Look into alternative negamaxes like C*

    int Quiesce(const int alpha, const int beta);

    #ifdef DEBUG
    // This is purely for debugging.
    uint64_t logDebugInfo() const;
    #endif

    private:
    bool isDraw() const;

    GameState& _state;
    ProtoBoard& _board;
};