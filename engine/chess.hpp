#pragma once
#include <enum.hpp>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

constexpr int BOARDSIZE = 8;
constexpr int NUMBER_OF_PIECEKIND = 16;

struct moveChunk{ //piece class의 부속 구조체로써만 동작해야 한다.
    private:
        threatType tT;
        std::pair<int, int> origin_of_directions;
        std::vector<std::pair<int, int>> diraction;
        int maxDistanse;
    public:
        moveChunk(){
            tT = threatType::NONE;
            origin_of_directions = {0, 0};
            diraction = {{0, 0}};
            maxDistanse = 0;
        }

        moveChunk(threatType t, std::pair<int, int> origin_of_dirs, std::vector<std::pair<int, int>> dirs) : 
        tT(t), origin_of_directions(origin_of_dirs), diraction(dirs){
            maxDistanse = BOARDSIZE;
        }

        moveChunk(threatType t, std::pair<int, int> origin_of_dirs, std::vector<std::pair<int, int>> dirs, int maxDist) : 
        tT(t), origin_of_directions(origin_of_dirs), diraction(dirs), maxDistanse(maxDist) {}

        //getter
        threatType getThreatType(){ return tT; }
        std::pair<int, int> getOrigin(){ return origin_of_directions; }
        std::vector<std::pair<int, int>> getDirs(){ return diraction; }
        int getMaxDistanse(){ return maxDistanse; }
};


struct piece{
    private:
        colorType cT;
        pieceType pT;
        int stun_stack;
        int move_stack;

        // 피스 설계때부터 정해지는 값들
        std::vector<moveChunk> mC; 
        bool isRoyal;
        bool isPromotable;
        std::vector<pieceType> promote_pool; // 프로모션할 때, 뭐로 변하는 지 그 풀을 정하는 변수다. 예시론 폰의 경우 퀸, 나이트, 비숍, 룩이 저장된다.
        std::vector<std::pair<int, int>> promotable_square; //예를 들어서, 폰의 경우 여기엔 1랭크(기물 색깔에 따라 다르겠다만)칸들의 좌표를 저장하면 된다. 그 칸에 도달하면 프로모션 하는 느낌으로.

        void setupMoveChunk(); //pieceType에 따라 그에 맞는 설정값을 부여
        void setupStunStack();
    public:
        piece(){
            cT = colorType::NONE;
            pT = pieceType::NONE;
            stun_stack = 0;
            move_stack = 0;
            mC.clear();
            isRoyal = false;
            isPromotable = false;
            promote_pool.clear();
            promotable_square.clear();
        }
        piece(colorType c, pieceType p) : cT(c), pT(p) {
            move_stack = 0;
            setupStunStack();
            setupMoveChunk();
        }
        piece(colorType c, pieceType p, int stun) : cT(c), pT(p), stun_stack(stun) {
            move_stack = 0;
            setupStunStack();
            setupMoveChunk();
        }
        piece(colorType c, pieceType p, int stun, int move) : cT(c), pT(p), stun_stack(stun), move_stack(move){
            setupMoveChunk();
        }

        void setupStunStackWithPosition(int file, int rank); //프로모션하는 기물을 위한 착수 위치를 고려한 스턴 스택 설정

        //getter
        colorType getColor() const { return cT; }
        pieceType getPieceType() const { return pT; }
        int getStun() const { return stun_stack; }
        int getMove() const { return move_stack; }
        std::vector<moveChunk> getMoveChunk() const { return mC; }
        bool getIsRoyal() const { return isRoyal; }
        bool getIsPromotable() const { return isPromotable; }
        std::vector<pieceType> getPromotePool() const { return promote_pool; }
        std::vector<std::pair<int, int>> getPromotableSquare() const { return promotable_square; }

        //setter
        void setStun(int s){
            if(s < 0) return;
            stun_stack = s; 
        }
        void setMove(int m){
            if(m < 0) return;
            move_stack = m; 
        }
        void setColor(colorType ct){ cT = ct; }
        void setRoyal(bool royalty){ isRoyal = royalty; }

        //스택 조작 핼퍼 함수
        void addStun(int ds){ 
            if(stun_stack + ds < 0) return;
            stun_stack += ds; 
        }
        void addOneStun() { stun_stack += 1; }
        void minusOneStun() { 
            if(stun_stack-1 < 0) return;
            stun_stack -= 1; 
        }

        void addMove(int dm){ 
            if(move_stack+dm < 0) return;
            move_stack += dm; 
        }
        void addOneMove() { move_stack += 1; }
        void minusOneMove() { 
            if(move_stack-1 < 0) return;
            move_stack -= 1; 
        }

        //기타 편의성 함수
        bool isEmpty() const {
            if(pT == pieceType::NONE) return true;
            else return false;
        }

        void clear(){
            cT = colorType::NONE;
            pT = pieceType::NONE;
            mC.clear();
            isRoyal = false;
            isPromotable = false;
            promote_pool.clear();
            promotable_square.clear();
        }
};

struct PGN{
    private:
        moveType mT;

        //moveType::MOVE
        int fromFile; //어느 칸에서
        int fromRank;
        threatType tT; //어떤 종류의 영향을
        int toFile; //어떤 칸에 행사했는가 를 뜻함.
        int toRank;

        colorType cT;
        pieceType pT;
    public:

        PGN(){
            mT = moveType::NONE;
            fromFile = 0;
            fromRank = 0;
            tT = threatType::NONE;
            toFile = 0;
            toRank = 0;
            cT  = colorType::NONE;
            pT = pieceType::NONE;
        }

        PGN(colorType ct, threatType tt, int fF, int fR, int tF, int tR) :
        mT(moveType::MOVE), fromFile(fF), fromRank(fR), tT(tt), toFile(tF), toRank(tR), cT(ct), pT(pieceType::NONE) { //이동 표현
        }

        PGN(colorType ct, threatType tt, int fF, int fR, int tF, int tR, pieceType pt) :
        mT(moveType::PROMOTE), fromFile(fF), fromRank(fR), tT(tt), toFile(tF), toRank(tR), cT(ct), pT(pt) { //승격 표현
        }

        PGN(colorType ct, int fF, int fR, moveType mt) :
        mT(mt), fromFile(fF), fromRank(fR), tT(threatType::NONE), toFile(0), toRank(0), cT(ct), pT(pieceType::NONE) { //계승 표현
        }

        PGN(colorType ct, int fF, int fR, pieceType pt) :
        mT(moveType::ADD), fromFile(fF), fromRank(fR), tT(threatType::NONE), toFile(0), toRank(0), cT(ct), pT(pt) { //착수 표현
        }

        bool operator==(const PGN& compare) const {
            if(mT != compare.mT) return false;
            if(fromFile != compare.fromFile) return false;
            if(fromRank != compare.fromRank) return false;
            if(tT != compare.tT) return false;
            if(toFile != compare.toFile) return false;
            if(toRank != compare.toRank) return false;
            if(pT != compare.pT) return false;

            return true;
        }

        //getter
        std::pair<int, int> getFromSquare() const {return {fromFile, fromRank};}
        std::pair<int, int> getToSquare() const {return {toFile, toRank};}
        threatType getThreatType() const {return tT;}
        moveType getMoveType() const {return mT;}
        pieceType getPieceType() const {return pT;}
        colorType getColorType() const {return cT;}
};


class chessboard{
    private:
        piece board[BOARDSIZE][BOARDSIZE];
        std::array<int, NUMBER_OF_PIECEKIND> whitePocket; //pieceType값을 인덱스로 사용함. 예시로 pieceType::KING == 0이니까 whitePocket[0] == 1이면 킹이 포켓에 1개 존재하는 거
        std::array<int, NUMBER_OF_PIECEKIND> blackPocket;
    public:
        chessboard(){
            whitePocket = {1, 1, 2, 2, 2, 8, //king queen bishop knight rook pwan
                0, //amazon
                0, //grasshopper
                0, //knightrider
                0, //archbishop
                0, //dababba
                0, //alfil
                0, //frez
                0, //centaur
                0, //camel
                0  //tempest rook
            };
            blackPocket = {1, 1, 2, 2, 2, 8, //king queen bishop knight rook pwan
                0, //amazon
                0, //grasshopper
                0, //knightrider
                0, //archbishop
                0, //dababba
                0, //alfil
                0, //frez
                0, //centaur
                0, //camel
                0  //tempest rook
            };
        }

        piece& operator()(int file, int rank){
            return board[file][rank];
        }

        const piece& at(int file, int rank) const {
            return board[file][rank];
        }

        //보드 조작 핼퍼 함수들
        bool isInBounds(int file, int rank) const; //보드 경계 체크
        void placePiece(colorType cT, pieceType pT, int file, int rank);
        void movePiece(int start_file, int start_rank, int end_file, int end_rank);
        void removePiece(int file, int rank);
        void succesionPiece(int file, int rank);
        void shiftPiece(int p1_file, int p1_rank, int p2_file, int p2_rank);
        void promotePiece(colorType cT, int file, int rank, pieceType promote);

        std::vector<PGN> calcLegalMovesInOnePiece(int file, int rank); //포지션에 따라 특정 기물의 합법 수를 계산 (이동 & 승격 PGN반환)
        std::vector<PGN> calcLegalPlacePiece();//특정 색상의 플레이어가 기물을 놓을 수 있는 착수 지점을 계산 (착수 PGN 반환)
        std::vector<PGN> calcLegalSuccesion();//승격 가능한 상태인지, 그리고 어떤 기물을 승격시킬 수 있는지를 계산 (계승 PGN반환)

        //행마법에 따라 보드를 조작하는 함수
        void updatePiece(PGN pgn); //기물의 threatType에 따라 보드 상태를 업데이트

        //디버그/테스트 함수들
        void displayBoard() const; //보드 상태 출력
        void displayPieceAt(int file, int rank) const; //특정 칸의 기물 정보 출력
        void displayPockets() const; //주머니(포켓) 정보 출력
        void displayPieceInfo(int file, int rank) const; //특정 칸의 기물 상세 정보 출력

        //스택이 색갈에 따라 일괄적용되는 규칙들을 구현할 함수.
        void pieceStackControllByColor(colorType cT, int d_stun, int d_move);

		//포켓 접근자
		const std::array<int, NUMBER_OF_PIECEKIND>& getWhitePocket() const { return whitePocket; }
		const std::array<int, NUMBER_OF_PIECEKIND>& getBlackPocket() const { return blackPocket; }
};
