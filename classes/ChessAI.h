#pragma once

#include "Chess.h"

class ChessAI {
    public:
    ChessAI(GameState);

    // Returns: positive value if AI wins, negative if human player wins, 0 for draw or undecided
    int evaluateBoard(const ProtoBoard&) const;

    // init like this : negamax(rootState, depth, -inf, +inf, 1)
    // player is the current player's number (AI or human)
    int negamax(ChessAI&, const int, const int, const int, const bool);

    // TODO: Look into alternative negamaxes like C*

    private:
    GameState& _state;
    ProtoBoard& _board;
};