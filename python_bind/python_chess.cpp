#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "chess.hpp"
#include "agent.hpp"

namespace py = pybind11;

PYBIND11_MODULE(chess_ext, m) {
	m.doc() = "pybind11 bindings for project_bc_refectoring chess engine (prototype)";

	// Enums
	py::enum_<pieceType>(m, "PieceType")
		.value("NONE", pieceType::NONE)
		.value("KING", pieceType::KING)
		.value("QUEEN", pieceType::QUEEN)
		.value("BISHOP", pieceType::BISHOP)
		.value("KNIGHT", pieceType::KNIGHT)
		.value("ROOK", pieceType::ROOK)
		.value("PWAN", pieceType::PWAN)
		.value("AMAZON", pieceType::AMAZON)
		.value("GRASSHOPPER", pieceType::GRASSHOPPER)
		.value("KNIGHTRIDER", pieceType::KNIGHTRIDER)
		.value("ARCHBISHOP", pieceType::ARCHBISHOP)
		.value("DABBABA", pieceType::DABBABA)
		.value("ALFIL", pieceType::ALFIL)
		.value("FERZ", pieceType::FERZ)
		.value("CENTAUR", pieceType::CENTAUR)
		.value("CAMEL", pieceType::CAMEL)
		.value("TEMPESTROOK", pieceType::TEMPESTROOK)
		.export_values();

	py::enum_<colorType>(m, "ColorType")
		.value("NONE", colorType::NONE)
		.value("WHITE", colorType::WHITE)
		.value("BLACK", colorType::BLACK)
		.export_values();

	py::enum_<threatType>(m, "ThreatType")
		.value("NONE", threatType::NONE)
		.value("CATCH", threatType::CATCH)
		.value("TAKE", threatType::TAKE)
		.value("MOVE", threatType::MOVE)
		.value("TAKEMOVE", threatType::TAKEMOVE)
		.value("TAKEJUMP", threatType::TAKEJUMP)
		.value("SHIFT", threatType::SHIFT)
		.export_values();

	py::enum_<moveType>(m, "MoveType")
		.value("NONE", moveType::NONE)
		.value("MOVE", moveType::MOVE)
		.value("ADD", moveType::ADD)
		.value("SUCCESION", moveType::SUCCESION)
		.value("PROMOTE", moveType::PROMOTE)
		.export_values();

	// PGN
	py::class_<PGN>(m, "PGN")
		.def(py::init<>())  // 기본 생성자
		.def(py::init<colorType, threatType, int, int, int, int>())  // 이동 표현
		.def(py::init<colorType, threatType, int, int, int, int, pieceType>())  // 승격 표현
		.def(py::init<colorType, int, int, moveType>())  // 계승 표현
		.def(py::init<colorType, int, int, pieceType>())  // 착수 표현
		.def("getFromSquare", &PGN::getFromSquare)
		.def("getToSquare", &PGN::getToSquare)
		.def("getThreatType", &PGN::getThreatType)
		.def("getMoveType", &PGN::getMoveType)
		.def("getPieceType", &PGN::getPieceType)
		.def("getColorType", &PGN::getColorType)
		.def("__repr__", [](const PGN &p){
			auto f = p.getFromSquare();
			auto t = p.getToSquare();
			return std::string("PGN(moveType=") + std::to_string(static_cast<int>(p.getMoveType())) +
				   ", color=" + std::to_string(static_cast<int>(p.getColorType())) +
				   ", from=(" + std::to_string(f.first) + "," + std::to_string(f.second) + ")" +
				   " -> (" + std::to_string(t.first) + "," + std::to_string(t.second) + ")" +
				   ", threatType=" + std::to_string(static_cast<int>(p.getThreatType())) +
				   ", pieceType=" + std::to_string(static_cast<int>(p.getPieceType())) + ")";
		});

	// Piece (minimal readonly/introspection)
	py::class_<piece>(m, "Piece")
		.def("getColor", &piece::getColor)
		.def("getPieceType", &piece::getPieceType)
		.def("getStun", &piece::getStun)
		.def("getMove", &piece::getMove)
		.def("getIsRoyal", &piece::getIsRoyal)
		.def("getIsPromotable", &piece::getIsPromotable)

		// setters and stack operations exposed for Python
		.def("setStun", &piece::setStun)
		.def("setMove", &piece::setMove)
		.def("addStun", &piece::addStun)
		.def("addOneStun", &piece::addOneStun)
		.def("minusOneStun", &piece::minusOneStun)
		.def("addMove", &piece::addMove)
		.def("addOneMove", &piece::addOneMove)
		.def("minusOneMove", &piece::minusOneMove)
		.def("setColor", &piece::setColor);

	// Chessboard
	py::class_<chessboard>(m, "ChessBoard")
		.def(py::init<>())
		// operator() -> __call__ (returns reference to piece; bind lifetime to board)
		.def("__call__", [](chessboard &b, int file, int rank) -> piece& { return b(file, rank); }, py::return_value_policy::reference_internal)
		// read-only indexer alternative: at(file,rank) returning copy
		.def("at", [](const chessboard &b, int file, int rank) { return b.at(file, rank); })
		.def("placePiece", &chessboard::placePiece, "Place a piece: (color, pieceType, file, rank)")
		.def("movePiece", &chessboard::movePiece, "Move piece: (start_file,start_rank,end_file,end_rank)")
		.def("removePiece", &chessboard::removePiece)
		.def("displayBoard", &chessboard::displayBoard)
		.def("displayPockets", &chessboard::displayPockets)
		.def("displayPieceAt", &chessboard::displayPieceAt)
		.def("displayPieceInfo", &chessboard::displayPieceInfo)
		.def("isInBounds", &chessboard::isInBounds)
		.def("calcLegalMovesInOnePiece", &chessboard::calcLegalMovesInOnePiece)
		.def("calcLegalPlacePiece", &chessboard::calcLegalPlacePiece)
		.def("calcLegalSuccesion", &chessboard::calcLegalSuccesion)
		.def("updatePiece", &chessboard::updatePiece)
		.def("pieceStackControllByColor", &chessboard::pieceStackControllByColor)
		.def("getWhitePocket", [](const chessboard &b) { return b.getWhitePocket(); })
		.def("getBlackPocket", [](const chessboard &b) { return b.getBlackPocket(); })
		.def("controllPocketValue", &chessboard::controllPocketValue)
		// Snapshot/restore for search without mutating live board
		.def("getPosition", [](const chessboard &b){
			// Convert internal position to Python-serializable dict
			auto pos = b.getPosition();
			py::dict out;
			py::list board; // list of rows
			for(int f=0; f<BOARDSIZE; ++f){
				py::list row;
				for(int r=0; r<BOARDSIZE; ++r){
					const piece &p = pos.board[f][r];
					if(p.getPieceType() == pieceType::NONE){
						row.append(py::none());
					}else{
						py::dict pd;
						pd["piece_type"] = static_cast<int>(p.getPieceType());
						pd["color"] = static_cast<int>(p.getColor());
						pd["stun"] = p.getStun();
						pd["move"] = p.getMove();
						pd["is_royal"] = p.getIsRoyal();
						row.append(pd);
					}
				}
				board.append(row);
			}
			py::list wp; py::list bp;
			for(int i=0;i<NUMBER_OF_PIECEKIND;++i){ wp.append(pos.whitePocket[i]); bp.append(pos.blackPocket[i]); }
			out["board"] = board;
			out["whitePocket"] = wp;
			out["blackPocket"] = bp;
			return out;
		})
		.def("setPosition", [](chessboard &b, py::dict d){
			position pos;
			py::list board = d["board"];
			for(int f=0; f<BOARDSIZE; ++f){
				py::list row = board[f];
				for(int r=0; r<BOARDSIZE; ++r){
					py::object cell = row[r];
					if(cell.is_none()){
						pos.board[f][r] = piece();
					}else{
						py::dict pd = cell.cast<py::dict>();
						pieceType pt = static_cast<pieceType>(pd["piece_type"].cast<int>());
						colorType ct = static_cast<colorType>(pd["color"].cast<int>());
						int stun = pd["stun"].cast<int>();
						int move = pd["move"].cast<int>();
						piece p(ct, pt, stun, move);
						if(pd.contains("is_royal") && pd["is_royal"].cast<bool>()) p.setRoyal(true);
						pos.board[f][r] = p;
					}
				}
			}
			py::list wp = d["whitePocket"];
			py::list bp = d["blackPocket"];
			for(int i=0;i<NUMBER_OF_PIECEKIND;++i){ pos.whitePocket[i] = wp[i].cast<int>(); pos.blackPocket[i] = bp[i].cast<int>(); }
			b.setPosition(pos);
		});

	// helper: expose pair<int,int> conversion automatically via stl

	// calcInfo (minimax aggregate result)
	py::class_<agent::calcInfo>(m, "CalcInfo")
		.def(py::init<>())
		.def_readwrite("eval_val", &agent::calcInfo::eval_val)
		.def_readwrite("line", &agent::calcInfo::line)
		.def_readwrite("bestMove", &agent::calcInfo::bestMove);

	// Bot bindings
	py::class_<agent::minimax>(m, "Minimax")
		.def(py::init<>())
		.def(py::init<colorType>())
		.def("setFollowTurn", &agent::minimax::setFollowTurn)
		.def("setPlacementSample", &agent::minimax::setPlacementSample)
		.def("reset_search_data", &agent::minimax::reset_search_data)
		.def("setIterativeDeepening", &agent::minimax::setIterativeDeepening)
		.def("setUseAspiration", &agent::minimax::setUseAspiration)
		.def("setAspirationWindowBase", &agent::minimax::setAspirationWindowBase)
		.def("setNodeSearched", &agent::minimax::setNodeSearched)
		.def("getNodesSearched", &agent::minimax::getNodesSearched)
		.def("eval_pos", &agent::minimax::eval_pos)
		.def("getBestMove", [](agent::minimax &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getBestMove(pos, depth); })
		.def("getBestLine", [](agent::minimax &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getBestLine(pos, depth); })
		.def("getCalcInfo", [](agent::minimax &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getCalcInfo(pos, depth); });

	py::class_<agent::minimax_GPTproposed>(m, "MinimaxGPT")
		.def(py::init<>())
		.def(py::init<colorType>())
		.def("setFollowTurn", &agent::minimax_GPTproposed::setFollowTurn)
		.def("setPlacementSample", &agent::minimax_GPTproposed::setPlacementSample)
		.def("reset_search_data", &agent::minimax_GPTproposed::reset_search_data)
		.def("setIterativeDeepening", &agent::minimax_GPTproposed::setIterativeDeepening)
		.def("setUseAspiration", &agent::minimax_GPTproposed::setUseAspiration)
		.def("setAspirationWindowBase", &agent::minimax_GPTproposed::setAspirationWindowBase)
		.def("setNodesSearched", &agent::minimax_GPTproposed::setNodesSearched)
		.def("getNodesSearched", &agent::minimax_GPTproposed::getNodesSearched)
		.def("eval_pos", &agent::minimax_GPTproposed::eval_pos)
		.def("getBestMove", [](agent::minimax_GPTproposed &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getBestMove(pos, depth); })
		.def("getBestLine", [](agent::minimax_GPTproposed &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getBestLine(pos, depth); })
		.def("getCalcInfo", [](agent::minimax_GPTproposed &bot, const chessboard &b, int depth){ position pos = b.getPosition(); return bot.getCalcInfo(pos, depth); });

}