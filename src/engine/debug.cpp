#include "chess.hpp"
#include <iomanip>

// 기물 타입을 문자로 변환
char getPieceSymbol(pieceType pT) {
    switch(pT) {
        case pieceType::KING:       return 'K';
        case pieceType::QUEEN:      return 'Q';
        case pieceType::BISHOP:     return 'B';
        case pieceType::KNIGHT:     return 'N';
        case pieceType::ROOK:       return 'R';
        case pieceType::PWAN:       return 'P';
        case pieceType::AMAZON:     return 'A';
        case pieceType::GRASSHOPPER: return 'G';
        case pieceType::KNIGHTRIDER: return 'H';
        case pieceType::ARCHBISHOP: return 'C';
        case pieceType::DABBABA:    return 'D';
        case pieceType::ALFIL:      return 'L';
        case pieceType::FERZ:       return 'F';
        case pieceType::CENTAUR:    return 'U';
        case pieceType::CAMEL:      return 'J';
        case pieceType::TEMPESTROOK: return 'S';
        case pieceType::NONE:       return '.';
        default:                    return '?';
    }
}

// 기물 타입을 이름으로 변환
std::string getPieceNameKor(pieceType pT) {
    switch(pT) {
        case pieceType::KING:       return "킹";
        case pieceType::QUEEN:      return "퀸";
        case pieceType::BISHOP:     return "비숍";
        case pieceType::KNIGHT:     return "나이트";
        case pieceType::ROOK:       return "룩";
        case pieceType::PWAN:       return "폰";
        case pieceType::AMAZON:     return "아마존";
        case pieceType::GRASSHOPPER: return "그래스호퍼";
        case pieceType::KNIGHTRIDER: return "나이트라이더";
        case pieceType::ARCHBISHOP: return "아크비숍";
        case pieceType::DABBABA:    return "다바바";
        case pieceType::ALFIL:      return "알필";
        case pieceType::FERZ:       return "페르즈";
        case pieceType::CENTAUR:    return "센타우르";
        case pieceType::CAMEL:      return "카멜";
        case pieceType::TEMPESTROOK: return "템페스트룩";
        case pieceType::NONE:       return "없음";
        default:                    return "미정의";
    }
}

// 보드 상태 출력
void chessboard::displayBoard() const {
    std::cout << "\n=== 체스보드 상태 ===" << std::endl;
    std::cout << "  a b c d e f g h" << std::endl;
    
    for(int rank = BOARDSIZE - 1; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for(int file = 0; file < BOARDSIZE; file++) {
            piece p = board[file][rank];
            char symbol = getPieceSymbol(p.getPieceType());
            
            if(p.getColor() == colorType::WHITE) {
                std::cout << symbol << " ";
            } else if(p.getColor() == colorType::BLACK) {
                std::cout << (char)std::tolower(symbol) << " ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << (rank + 1) << std::endl;
    }
    std::cout << "  a b c d e f g h\n" << std::endl;
}

// 특정 칸의 기물 정보 출력
void chessboard::displayPieceAt(int file, int rank) const {
    if(!isInBounds(file, rank)) {
        std::cout << "범위를 벗어난 좌표입니다." << std::endl;
        return;
    }
    
    piece p = board[file][rank];
    
    std::cout << "\n칸 (" << (char)('a' + file) << (rank + 1) << ")의 기물 정보:" << std::endl;
    
    if(p.isEmpty()) {
        std::cout << "  비어있습니다." << std::endl;
    } else {
        std::cout << "  색상: " << (p.getColor() == colorType::WHITE ? "백" : "흑") << std::endl;
        std::cout << "  기물: " << getPieceNameKor(p.getPieceType()) << std::endl;
        std::cout << "  스턴 스택: " << p.getStun() << std::endl;
        std::cout << "  이동 스택: " << p.getMove() << std::endl;
        std::cout << "  로얄 피스: " << (p.getIsRoyal() ? "예" : "아니오") << std::endl;
        std::cout << "  프로모션 가능: " << (p.getIsPromotable() ? "예" : "아니오") << std::endl;
    }
    std::cout << std::endl;
}

// 주머니(포켓) 정보 출력
void chessboard::displayPockets() const {
    std::cout << "\n=== 포켓 정보 ===" << std::endl;
    
    std::cout << "백 포켓:" << std::endl;
    for(int i = 0; i < NUMBER_OF_PIECEKIND; i++) {
        if(whitePocket[i] > 0) {
            std::cout << "  " << getPieceNameKor(static_cast<pieceType>(i)) 
                      << ": " << whitePocket[i] << "개" << std::endl;
        }
    }
    
    std::cout << "\n흑 포켓:" << std::endl;
    for(int i = 0; i < NUMBER_OF_PIECEKIND; i++) {
        if(blackPocket[i] > 0) {
            std::cout << "  " << getPieceNameKor(static_cast<pieceType>(i)) 
                      << ": " << blackPocket[i] << "개" << std::endl;
        }
    }
    std::cout << std::endl;
}

// 특정 칸의 기물 상세 정보 출력
void chessboard::displayPieceInfo(int file, int rank) const {
    if(!isInBounds(file, rank)) {
        std::cout << "범위를 벗어난 좌표입니다." << std::endl;
        return;
    }
    
    piece p = board[file][rank];
    
    std::cout << "\n칸 (" << (char)('a' + file) << (rank + 1) << ")의 상세 정보:" << std::endl;
    
    if(p.isEmpty()) {
        std::cout << "  비어있습니다." << std::endl;
    } else {
        std::cout << "  색상: " << (p.getColor() == colorType::WHITE ? "백" : "흑") << std::endl;
        std::cout << "  기물: " << getPieceNameKor(p.getPieceType()) << std::endl;
        std::cout << "  스턴 스택: " << p.getStun() << std::endl;
        std::cout << "  이동 스택: " << p.getMove() << std::endl;
        std::cout << "  로얄 피스: " << (p.getIsRoyal() ? "예" : "아니오") << std::endl;
        std::cout << "  프로모션 가능: " << (p.getIsPromotable() ? "예" : "아니오") << std::endl;
        
        auto moveChunks = p.getMoveChunk();
        std::cout << "  행마 청크 개수: " << moveChunks.size() << std::endl;
        
        for(size_t i = 0; i < moveChunks.size(); i++) {
            std::cout << "    청크 #" << (i + 1) << ":" << std::endl;
            std::cout << "      최대 거리: " << moveChunks[i].getMaxDistanse() << std::endl;
            auto dirs = moveChunks[i].getDirs();
            std::cout << "      방향 개수: " << dirs.size() << std::endl;
        }
    }
    std::cout << std::endl;
}
