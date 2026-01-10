#include "chess.hpp"

// piece::setupMoveChunk() 구현 (경량화: 규칙은 PieceSpecRegistry에서 관리)
void piece::setupRoyal() {
    // 인스턴스 기본 로열티만 설정 (규칙상 동적 변경 가능)
    isRoyal = (pT == pieceType::KING);
}

void piece::setupStunStack(){
    switch (pT){
        case pieceType::KING:
        case pieceType::GRASSHOPPER:
            stun_stack = 4;
            break;
        case pieceType::QUEEN:
            stun_stack = 9;
            break;
        case pieceType::ROOK:
        case pieceType::CENTAUR:
            stun_stack = 5;
            break;
        case pieceType::BISHOP:
        case pieceType::KNIGHT:
        case pieceType::CAMEL:
            stun_stack = 3;
            break;
        case pieceType::TEMPESTROOK:
        case pieceType::KNIGHTRIDER:
            stun_stack = 7;
            break;
        case pieceType::ARCHBISHOP:
            stun_stack = 6;
            break;
        case pieceType::DABBABA:
        case pieceType::ALFIL:
            stun_stack = 2;
            break;
        case pieceType::AMAZON:
            stun_stack = 13;
            break;
        case pieceType::FERZ:
            stun_stack = 1;
            break;
        case pieceType::SAMURAI:
            stun_stack = 8;
            break;
        case pieceType::PWAN:
            stun_stack = 1; // 기본값, 실제로는 setupStunStackWithPosition에서 위치 기반으로 재설정됨
            break;
        default:
            break;
    }
}

void piece::setupStunStackWithPosition(int file, int rank){
    // 기본 스턴 스택 설정
    setupStunStack();

    // 프로모션 가능한 기물(현재는 폰만)에 대해 위치 기반 조정
    switch (pT) {
        case pieceType::PWAN: {
            // 백 기준: 랭크 1(인덱스 0) = 8 스택, 랭크 2(인덱스 1) = 7 스택, ..., 랭크 7(인덱스 6) = 2 스택
            // 흑 기준: 랭크 8(인덱스 7) = 8 스택, 랭크 7(인덱스 6) = 7 스택, ..., 랭크 2(인덱스 1) = 2 스택
            if(cT == colorType::WHITE){
                // 백 폰: rank가 0이면 8, 1이면 7, ..., 6이면 2
                stun_stack = 8 - rank;
            }else{
                // 흑 폰: rank가 7이면 8, 6이면 7, ..., 1이면 2
                // equivalent to: stun_stack = rank + 1;
                stun_stack = 8 - (7 - rank);
            }
            break;
        }
        default:
            break;
    }
    // 향후 다른 프로모션 가능 기물 추가 시 여기에 추가
}
