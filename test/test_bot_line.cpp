#include <iostream>
#include <agent.hpp>
#include <chess.hpp>

static const char* pieceTypeToStr(pieceType pt){
    switch(pt){
        case pieceType::KING: return "K";
        case pieceType::QUEEN: return "Q";
        case pieceType::ROOK: return "R";
        case pieceType::BISHOP: return "B";
        case pieceType::KNIGHT: return "N";
        case pieceType::PWAN: return "P";
        case pieceType::AMAZON: return "A";
        case pieceType::GRASSHOPPER: return "G";
        case pieceType::KNIGHTRIDER: return "Kr";
        case pieceType::ARCHBISHOP: return "Ab";
        case pieceType::DABBABA: return "D";
        case pieceType::ALFIL: return "L";
        case pieceType::FERZ: return "F";
        case pieceType::CENTAUR: return "C";
        case pieceType::CAMEL: return "Cl";
        case pieceType::TEMPESTROOK: return "Tr";
        default: return "?";
    }
}

static const char* colorTypeToStr(colorType ct){
    switch (ct)
    {
        case colorType::WHITE:
            return "w";
        case colorType::BLACK:
            return "b";
        default:
            return "?";
    }
}


int main(){
    // Create a valid default position via chessboard default constructor
    chessboard cb_default;
    cb_default.setVarientPiece();
    cb_default.placePiece(colorType::WHITE, pieceType::KING, 4, 3);
    position pos = cb_default.getPosition();
    // default-constructed board array contains piece::NONE; pockets keep initial values

    // Create minimax bot to play as WHITE
    agent::minimax_GPTproposed bot(pos, colorType::BLACK);
    bot.setIterativeDeepening(true);
    bot.setUseAspiration(false);
    bot.setPlacementSample(5);

    // Search depth (small for test)
    int depth = 12;

    auto line = bot.getBestLine(pos, depth);

    std::cout << "Best line (log + continuation) size=" << line.size() << "\n";
    for(size_t i=0;i<line.size();++i){
        const PGN &m = line[i];
        std::string move_str;
        if(m.getMoveType() == moveType::NONE) move_str = "NO_MOVE";
        else if(m.getMoveType() == moveType::ADD){ auto f=m.getFromSquare(); move_str = colorTypeToStr(m.getColorType()) + std::string("ADD ") + pieceTypeToStr(m.getPieceType()) + " at(" + std::to_string(f.first)+","+std::to_string(f.second)+")"; }
        else if(m.getMoveType() == moveType::SUCCESION){auto f=m.getFromSquare(); move_str = colorTypeToStr(m.getColorType()) + std::string("SUCESSION ") + "at(" + std::to_string(f.first)+","+std::to_string(f.second)+")";}
        else { auto f=m.getFromSquare(); auto t=m.getToSquare(); move_str = colorTypeToStr(m.getColorType());  move_str += "from("+std::to_string(f.first)+","+std::to_string(f.second)+")->("+std::to_string(t.first)+","+std::to_string(t.second)+")"; }
        if(m.getMoveType() == moveType::PROMOTE){ move_str += " promote_to="; move_str += pieceTypeToStr(m.getPieceType()); }
        std::cout << move_str;
        std::cout << "\n";
    }

    return 0;
}
