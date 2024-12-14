#include "Application.h"
#include "imgui/imgui.h"
#include "classes/Chess.h"
#include "tools/Logger.h"

namespace ClassGame {
	Chess *game = nullptr;
	bool gameOver = false;
	int gameWinner = -1;

	// game starting point
	// this is called by the main render loop in main.cpp
	void GameStartUp() {
		game = new Chess();
		game->setUpBoard();
	}

	void drawMoveProber() {
		const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
							ImGuiTableFlags_RowBg;

		ImGui::BeginChild("Moves", ImVec2(0, 0), true); 
		if (ImGui::BeginTable("Move Prober", 2, flags)) {
			ImGui::TableSetupColumn("I", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 12.0f);
			ImGui::TableSetupColumn("Moves", ImGuiTableColumnFlags_WidthFixed  | ImGuiTableColumnFlags_DefaultSort);
			ImGui::TableHeadersRow();

			std::unordered_map<uint8_t, std::vector<Move>> moves = game->getMoves();
			std::vector<std::pair<int, std::vector<Move>>> moveList;
			moveList.reserve(64);
			for (int i = 0; i < 64; i++) {
				moveList.emplace_back(i, moves[i]);
			}

			for(const std::pair<int, std::vector<Move>>& data : moveList) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", ChessSquare::indexToPosNotation(data.first).c_str());

				ImGui::TableSetColumnIndex(1);
				std::string moves;
				for (Move i : data.second) {
					moves += ChessSquare::indexToPosNotation(i.getTo()) + " ";
				}
				ImGui::Text("%s", moves.c_str());
			}

			ImGui::EndTable();
		}
		ImGui::EndChild();
	}

	const std::map<ChessPiece, char> symbolFromPiece = {
		{ChessPiece::Pawn, 'p'},
		{ChessPiece::Knight, 'n'},
		{ChessPiece::Bishop, 'b'},
		{ChessPiece::Rook, 'r'},
		{ChessPiece::Queen, 'q'},
		{ChessPiece::King, 'k'}
	};

	void drawState() {
		ImGui::BeginChild("State", ImVec2(300, 400), true);

		GameState state = game->getState();

		uint8_t castling = state.getCastlingRights();
		std::string castleString;
		if ((castling & (1 << 3)) != 0) {
			castleString += 'K';
		}
		if ((castling & (1 << 2)) != 0) {
			castleString += 'Q';
		}
		if ((castling & (1 << 1)) != 0) {
			castleString += 'k';
		}
		if ((castling & (1 << 0)) != 0) {
			castleString += 'q';
		}
		if (castling == 0) {
			castleString += '-';
		}

		ImGui::Text("Game State");
		ImGui::Text("Clock: %d", state.getClock());
		ImGui::Text("Half Clock: %d", state.getHalfClock());
		ImGui::Text("Black's Turn? %d", state.isBlackTurn());
		ImGui::Text("En Passant Square: %s", ChessSquare::indexToPosNotation(state.getEnPassantSquare()).c_str());
		ImGui::Text("Castle Rights %s", castleString.c_str());

		std::string stateStr = "";
		ProtoBoard bitboard = state.getProtoBoard();

		int file = 8;
		int rank = 0;
		for (int k = 0; k < 64; k++) {
			if (k % 8 == 0) {
				stateStr += '\n';
				file--;
				rank = 0;
			}

			ChessPiece piece = bitboard.PieceFromIndex(file * 8 + rank);
			if (piece == ChessPiece::NoPiece) {
				stateStr += '0';
				rank++;
				continue;
			}
			char symbol = symbolFromPiece.at((ChessPiece)(piece & 7));
			if ((piece & 8) == 0) {
				symbol = toupper(symbol);
			}

			stateStr += symbol;
			rank++;
		}
		ImGui::Text("%s", stateStr.c_str());

		for (int i = 0; i < 12; i++) {
			std::string s;

			int piece = 0;

			if (i < 6) {
				s += 'w';
			} else {
				s += 'b';
				piece = 8;
			}

			int j = i < 6 ?  i : i - 6;
			switch (j) {
				case 0:
					s += "P ";
					piece |= 0b001;
					break;
				case 1:
					s += "Kn";
					piece |= 0b010;
					break;
				case 2:
					s += "B ";
					piece |= 0b011;
					break;
				case 3:
					s += "R ";
					piece |= 0b100;
					break;
				case 4:
					s += "Q ";
					piece |= 0b101;
					break;
				case 5:
					s += "K ";
					piece |= 0b110;
					break;
			}
			s += " -- ";

			auto pos = bitboard.getBitPositions((ChessPiece)piece);
			for (int i : pos) {
				s += ChessSquare::indexToPosNotation(i) + " ";
			}
			ImGui::Text("%s", s.c_str());
		}
		
		ImGui::EndChild();
	}

	// game render loop
	// this is called by the main render loop in main.cpp
	void RenderGame() {
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		//ImGui::ShowDemoWindow();

		ImGui::Begin("Settings");
		ImGui::Text("Current Player Number: %d", game->getCurrentPlayer()->playerNumber());
		ImGui::Text("Current Board State: %s", game->stateString().c_str());

		if (gameOver) {
			ImGui::Text("\nGame Over!");
			ImGui::Text("Winner: %d", gameWinner);
			if (ImGui::Button("Reset Game")) {
				game->stopGame();
				game->setUpBoard();
				gameOver = false;
				gameWinner = -1;
			}
		}

		drawState();
		drawMoveProber();

		ImGui::End();

		ImGui::Begin("GameWindow");
		game->drawFrame();
		ImGui::End();
		#ifdef DEBUG
		Loggy.draw();
		#endif
	}

	// end turn is called by the game code at the end of each turn
	// this is where we check for a winner
	void EndOfTurn() {
		Player *winner = game->checkForWinner();
		if (winner) {
			gameOver = true;
			gameWinner = winner->playerNumber();
		}
		if (game->checkForDraw()) {
			gameOver = true;
			gameWinner = -1;
		}
	}
}
