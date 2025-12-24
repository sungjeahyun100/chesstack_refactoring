#include "chess.hpp"

// 방향 벡터 상수 오프셋들
const std::vector<std::pair<int, int>> KNIGHT_DIRECTIONS = {
    {1, 2}, {2, 1}, {2, -1}, {1, -2},
    {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
};

const std::vector<std::pair<int, int>> BISHOP_DIRECTIONS = {
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1}
};

const std::vector<std::pair<int, int>> ROOK_DIRECTIONS = {
    {0, 1}, {1, 0}, {0, -1}, {-1, 0}
};

const std::vector<std::pair<int, int>> EIGHT_WAY_DIRECTIONS = {
    {0, 1}, {1, 1}, {1, 0}, {1, -1},
    {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
};

const std::vector<std::pair<int, int>> DABBABA_DIRECTIONS = {
    {0, 2}, {2, 0}, {0, -2}, {-2, 0}
};

const std::vector<std::pair<int, int>> ALFIL_DIRECTIONS = {
    {2, 2}, {2, -2}, {-2, -2}, {-2, 2}
};

const std::vector<std::pair<int, int>> CAMEL_DIRECTIONS = {
    {1, 3}, {3, 1}, {3, -1}, {1, -3},
    {-1, -3}, {-3, -1}, {-3, 1}, {-1, 3}
};

// piece::setupMoveChunk() 구현
void piece::setupMoveChunk() {
    // 기본값 초기화
    mC.clear();
    isRoyal = false;
    isPromotable = false;
    promote_pool.clear();
    promotable_square.clear();

    switch(pT) {
        case pieceType::KING:
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, EIGHT_WAY_DIRECTIONS, 1));
            isRoyal = true;
            break;

        case pieceType::QUEEN:
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, EIGHT_WAY_DIRECTIONS));
            break;

        case pieceType::BISHOP:
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, BISHOP_DIRECTIONS));
            break;

        case pieceType::KNIGHT:
           mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, KNIGHT_DIRECTIONS, 1));
            break;

        case pieceType::ROOK:
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, ROOK_DIRECTIONS));
            break;

        case pieceType::PWAN:
            {
                // 폰: 색상에 따라 방향 다름
                if(cT == colorType::WHITE) {
                    // 백 폰: 앞으로 1칸 이동
                    mC.push_back(moveChunk(threatType::MOVE, {0, 0}, {{0, 1}}, 1));
                    // 대각선 캡처
                    mC.push_back(moveChunk(threatType::TAKE, {0, 0}, {{-1, 1}, {1, 1}}, 1));
                } else {
                    // 흑 폰: 아래로 1칸 이동
                    mC.push_back(moveChunk(threatType::MOVE, {0, 0}, {{0, -1}}, 1));
                    // 대각선 캡처
                    mC.push_back(moveChunk(threatType::TAKE, {0, 0}, {{-1, -1}, {1, -1}}, 1));
                }
                isPromotable = true;
                promote_pool = {pieceType::QUEEN, pieceType::ROOK, pieceType::BISHOP, pieceType::KNIGHT};
                // 프로모션 가능 칸 (8번째 랭크 또는 1번째 랭크)
                int targetRank = (cT == colorType::WHITE) ? 7 : 0;
                for(int file = 0; file < BOARDSIZE; file++) {
                    promotable_square.push_back({file, targetRank});
                }
            }
            break;

        case pieceType::AMAZON:
            // 아마존: 퀸 + 나이트
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, KNIGHT_DIRECTIONS, 1));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, EIGHT_WAY_DIRECTIONS));
            break;

        case pieceType::GRASSHOPPER:
            // 그래스호퍼: 기물을 뛰어넘어 이동
            mC.push_back(moveChunk(threatType::TAKEJUMP, {0, 0}, EIGHT_WAY_DIRECTIONS));
            break;

        case pieceType::KNIGHTRIDER:
            // 나이트라이더: 나이트 방향으로 무한 이동
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, KNIGHT_DIRECTIONS));
            break;

        case pieceType::ARCHBISHOP:
            // 아크비숍: 비숍 + 나이트
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, KNIGHT_DIRECTIONS, 1));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, BISHOP_DIRECTIONS));
            break;

        case pieceType::DABBABA:
            // 다바바: 2칸 직선 점프
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, DABBABA_DIRECTIONS, 1));
            break;

        case pieceType::ALFIL:
            // 알필: 2칸 대각선 점프
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, ALFIL_DIRECTIONS, 1));
            break;

        case pieceType::FERZ:
            // 페르즈: 1칸 대각선
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, BISHOP_DIRECTIONS, 1));
            break;

        case pieceType::CENTAUR:
            // 센타우르: 킹 + 나이트
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, EIGHT_WAY_DIRECTIONS, 1));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, KNIGHT_DIRECTIONS, 1));
            break;

        case pieceType::CAMEL:
            // 카멜: (1,3) 점프
            mC.push_back(moveChunk(threatType::TAKEMOVE, {0, 0}, CAMEL_DIRECTIONS, 1));
            break;

        case pieceType::TEMPESTROOK: //이 기물은 각 모퉁이에 룩을 붙여놓은거 같은 행마법을 가진다. 따라서
            mC.push_back(moveChunk(threatType::TAKEMOVE, {1, 1}, {{0, 1}, {1, 0}}));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {1, -1}, {{1, 0}, {0, -1}}));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {-1, 1}, {{0, 1}, {-1, 0}}));
            mC.push_back(moveChunk(threatType::TAKEMOVE, {-1, -1}, {{0, -1}, {-1, 0}}));
            break;

        default:
            break;
    }
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
