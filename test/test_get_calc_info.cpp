#include <agent.hpp>
#include <chess.hpp>

#include <iostream>
#include <string>

using namespace agent;

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

static const char* moveTypeToStr(moveType mt){
    switch(mt){
        case moveType::MOVE: return "MOVE";
        case moveType::ADD: return "ADD";
        case moveType::PROMOTE: return "PROMOTE";
        case moveType::SUCCESION: return "SUCCESION";
        case moveType::DISGUISE: return "DISGUISE";
        case moveType::NONE: return "NONE";
        default: return "?";
    }
}

static const char* colorTypeToStr(colorType ct){
    switch(ct){
        case colorType::WHITE: return "w";
        case colorType::BLACK: return "b";
        default: return "?";
    }
}

// 표준 체스 초기 포지션을 만들되, 포켓은 모두 비워 착수 PGN이 생성되지 않도록 한다.
static position build_initial_position_without_pockets(){
    chessboard cb;

    // 백 기본 배치
    cb.placePiece(colorType::WHITE, pieceType::ROOK, 0, 0);
    cb.placePiece(colorType::WHITE, pieceType::KNIGHT, 1, 0);
    cb.placePiece(colorType::WHITE, pieceType::BISHOP, 2, 0);
    cb.placePiece(colorType::WHITE, pieceType::QUEEN, 3, 0);
    cb.placePiece(colorType::WHITE, pieceType::KING, 4, 0);
    cb.placePiece(colorType::WHITE, pieceType::BISHOP, 5, 0);
    cb.placePiece(colorType::WHITE, pieceType::KNIGHT, 6, 0);
    cb.placePiece(colorType::WHITE, pieceType::ROOK, 7, 0);
    for(int f=0; f<BOARDSIZE; ++f){
        cb.placePiece(colorType::WHITE, pieceType::PWAN, f, 1);
    }

    // 흑 기본 배치
    cb.placePiece(colorType::BLACK, pieceType::ROOK, 0, 7);
    cb.placePiece(colorType::BLACK, pieceType::KNIGHT, 1, 7);
    cb.placePiece(colorType::BLACK, pieceType::BISHOP, 2, 7);
    cb.placePiece(colorType::BLACK, pieceType::QUEEN, 3, 7);
    cb.placePiece(colorType::BLACK, pieceType::KING, 4, 7);
    cb.placePiece(colorType::BLACK, pieceType::BISHOP, 5, 7);
    cb.placePiece(colorType::BLACK, pieceType::KNIGHT, 6, 7);
    cb.placePiece(colorType::BLACK, pieceType::ROOK, 7, 7);
    for(int f=0; f<BOARDSIZE; ++f){
        cb.placePiece(colorType::BLACK, pieceType::PWAN, f, 6);
    }

    // 이동/스턴 스택을 기본값으로 세팅해 검색이 가능하도록 보정
    for(int f=0; f<BOARDSIZE; ++f){
        for(int r=0; r<BOARDSIZE; ++r){
            if(!cb(f,r).isEmpty()){
                cb(f,r).setMove(10);
                cb(f,r).setStun(0);
            }
        }
    }

    position pos = cb.getPosition();
    pos.turn_right = colorType::WHITE;
    return pos;
}

int main(){
    chessboard cb_default;
    cb_default.updatePiece(PGN(colorType::WHITE, 4, 0, pieceType::KING));
    cb_default.updatePiece(PGN(colorType::BLACK, 4, 7, pieceType::KING));
    //cb_default(4,0).setMove(10);
    //cb_default(4,0).setStun(0);
    //cb_default(4,7).setMove(10);
    //cb_default(4,7).setStun(0);

    position start_default = cb_default.getPosition();
    

    minimax_GPTproposed bot(colorType::WHITE);
    bot.setFollowTurn(true);
    bot.setIterativeDeepening(true);
    bot.setPlacementSample(6); // 포켓을 비웠으므로 착수 시도 비활성화

    const int depth = 5; // 초깊이로 빠르게 검증
    calcInfo info = bot.getCalcInfo(start_default, depth);

    std::cout << "getCalcInfo on initial position\n";
    std::cout << "depth=" << depth << " eval=" << info.eval_val << "\n";

    if(info.bestMove.getMoveType() != moveType::NONE){
        auto f = info.bestMove.getFromSquare();
        auto t = info.bestMove.getToSquare();
        std::cout << "bestMove=" << moveTypeToStr(info.bestMove.getMoveType())
                  << " " << colorTypeToStr(info.bestMove.getColorType())
                  << " " << pieceTypeToStr(info.bestMove.getPieceType())
                  << " from(" << f.first << "," << f.second << ")"
                  << " to(" << t.first << "," << t.second << ")" << "\n";
    } else {
        std::cout << "bestMove=NONE\n";
    }

    std::cout << "line length=" << info.line.size() << "\n";
    for(size_t i=0;i<info.line.size();++i){
        const PGN &m = info.line[i];
        auto f = m.getFromSquare();
        auto t = m.getToSquare();
        std::cout << i+1 << ": " << moveTypeToStr(m.getMoveType())
                  << " " << colorTypeToStr(m.getColorType())
                  << " " << pieceTypeToStr(m.getPieceType())
                  << " from(" << f.first << "," << f.second << ")"
                  << " to(" << t.first << "," << t.second << ")";
        if(m.getMoveType() == moveType::PROMOTE){
            std::cout << " promote_to=" << pieceTypeToStr(m.getPieceType());
        }
        std::cout << "\n";
    }

    return 0;
}
