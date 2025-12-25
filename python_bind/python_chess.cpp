#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "chess.hpp"

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
		.def("getBlackPocket", [](const chessboard &b) { return b.getBlackPocket(); });

	// helper: expose pair<int,int> conversion automatically via stl
}