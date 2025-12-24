#include "chess.hpp"

bool chessboard::isInBounds(int file, int rank) const
{
    return (file >= 0 && file < BOARDSIZE && rank >= 0 && rank < BOARDSIZE);
}

void chessboard::placePiece(colorType cT, pieceType pT, int file, int rank)
{
    if(board[file][rank].isEmpty() == false){
        std::cout << "this square is not empty." << std::endl;
        return;
    }

    if(cT == colorType::WHITE){
        if(whitePocket[static_cast<int>(pT)] <= 0){
            std::cout << "is not in your pocket, white" << std::endl;
            return;
        }else{
            piece p = piece(cT, pT);
            if(p.getIsPromotable()){
                for(auto& promotable_square : p.getPromotableSquare()){
                    if(promotable_square == std::pair<int, int>(file, rank)){
                        std::cout << "this square is promotable square. not allowed" << std::endl;
                        return;
                    }
                }
            }

            whitePocket[static_cast<int>(pT)] -= 1;
            board[file][rank] = piece(cT, pT);
            // 착수 위치를 고려한 스턴 스택 설정 (폰 등 프로모션 가능 기물)
            board[file][rank].setupStunStackWithPosition(file, rank);
        }
    }else{ //colorType::BLACK
        if(blackPocket[static_cast<int>(pT)] <= 0){
            std::cout << "is not in your pocket, black" << std::endl;
            return;
        }else{
            piece p = piece(cT, pT);
            if(p.getIsPromotable()){
                for(auto& promotable_square : p.getPromotableSquare()){
                    if(promotable_square == std::pair<int, int>(file, rank)){
                        std::cout << "this square is promotable square. not allowed" << std::endl;
                        return;
                    }
                }
            }

            blackPocket[static_cast<int>(pT)] -= 1;
            board[file][rank] = piece(cT, pT);
            // 착수 위치를 고려한 스턴 스택 설정 (폰 등 프로모션 가능 기물)
            board[file][rank].setupStunStackWithPosition(file, rank);
        }
    }
}

void chessboard::movePiece(int start_file, int start_rank, int end_file, int end_rank)
{
    if(board[start_file][start_rank].isEmpty() == true){
        std::cout << "that square has no pieces." << std::endl;
        return;
    }

    board[end_file][end_rank] = board[start_file][start_rank];
    board[start_file][start_rank].clear();
}

void chessboard::removePiece(int file, int rank)
{
    board[file][rank].clear();
}

void chessboard::succesionPiece(int file, int rank)
{
    board[file][rank].setRoyal(true);
}

void chessboard::shiftPiece(int p1_file, int p1_rank, int p2_file, int p2_rank)
{
    if(board[p1_file][p1_rank].isEmpty() == true){
        std::cout << "that p1 square has no pieces." << std::endl;
        return;
    }
    if(board[p2_file][p2_rank].isEmpty() == true){
        std::cout << "that p2 square has no pieces." << std::endl;
        return;
    }

    piece buffur;

    buffur = board[p2_file][p2_rank];
    board[p2_file][p2_rank] = board[p1_file][p1_rank];
    board[p1_file][p1_rank] = buffur;
}

void chessboard::promotePiece(colorType cT, int file, int rank, pieceType promote)
{
    if(board[file][rank].getIsPromotable() == false){
        std::cout << "that piece is not promotable" << std::endl;
        return;
    }

    for(auto& promotable_square : board[file][rank].getPromotableSquare()){
        if(promotable_square.first == file && promotable_square.second == rank){
            board[file][rank] = piece(cT, promote);
        }
    }
}

std::vector<PGN> chessboard::calcLegalMovesInOnePiece(int file, int rank)
{
    std::vector<PGN> result;
    result.clear();

    if(board[file][rank].isEmpty()){
        std::cout << "there has no pieces." << std::endl;
        return std::vector<PGN>();
    }

    auto current_piece = board[file][rank];

    if(current_piece.getStun() > 0){
        std::cout << "that piece is stunned." << std::endl;
        return std::vector<PGN>();
    }

    if(current_piece.getMove() == 0){
        std::cout << "that piece has no move stack." << std::endl;
        return std::vector<PGN>();
    }

    for(auto& mC : current_piece.getMoveChunk()){
        auto origin = mC.getOrigin();
        auto dirs = mC.getDirs();
        int maxDist = mC.getMaxDistanse();
        auto threat_type = mC.getThreatType();

        int origin_file = file+origin.first;
        int origin_rank = rank+origin.second; 

        switch (threat_type){
            case threatType::CATCH:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.isEmpty()) continue; //칸이 비어있다면
                        if(next_square.getColor() == current_piece.getColor()) break; //칸에 아군이 있다면
                        if(next_square.getColor() != current_piece.getColor()) {
                            result.push_back(PGN(threatType::CATCH, origin_file, origin_rank, next_square_file, next_square_rank)); // 칸에 적 기물이 있다면
                            break;
                        }
                    }
                }
                break;
            case threatType::TAKEMOVE:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.isEmpty()){
                            result.push_back(PGN(threatType::TAKEMOVE, origin_file, origin_rank, next_square_file, next_square_rank)); //칸이 비어있다면
                            continue;
                        }
                        if(next_square.getColor() == current_piece.getColor()) break; //칸에 아군이 있다면
                        if(next_square.getColor() != current_piece.getColor()) {
                            result.push_back(PGN(threatType::TAKEMOVE, origin_file, origin_rank, next_square_file, next_square_rank)); // 칸에 적 기물이 있다면
                            break;
                        }
                    }
                }
                break;
            case threatType::MOVE:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.getColor() == current_piece.getColor()) break; //칸에 아군이 있다면
                        if(next_square.isEmpty()){
                            result.push_back(PGN(threatType::MOVE, origin_file, origin_rank, next_square_file, next_square_rank)); //칸이 비어있다면
                            continue;
                        }
                        if(next_square.getColor() != current_piece.getColor()) {
                            break; // 칸에 적 기물이 있다면
                        }
                    }
                }
                break;
            case threatType::SHIFT:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.getColor() == current_piece.getColor()) {
                            result.push_back(PGN(threatType::SHIFT, origin_file, origin_rank, next_square_file, next_square_rank));//칸에 아군이 있다면
                            break;
                        }
                        if(next_square.isEmpty()){
                            continue; //칸이 비어있다면
                        }
                        if(next_square.getColor() != current_piece.getColor()) {
                            result.push_back(PGN(threatType::SHIFT, origin_file, origin_rank, next_square_file, next_square_rank));// 칸에 적 기물이 있다면
                            break;
                        }
                    }
                }
                break;
            case threatType::TAKE:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.getColor() == current_piece.getColor()) {
                            break;//칸에 아군이 있다면
                        }
                        if(next_square.isEmpty()){
                            continue; //칸이 비어있다면
                        }
                        if(next_square.getColor() != current_piece.getColor()) {// 칸에 적 기물이 있다면
                            result.push_back(PGN(threatType::TAKE, origin_file, origin_rank, next_square_file, next_square_rank));
                            break;
                        }
                    }
                }
                break;
            case threatType::TAKEJUMP:
                for(auto& dir : dirs){
                    for(int i = 1; i <= maxDist; i++){
                        int next_square_file = origin_file+(dir.first*i);
                        int next_square_rank = origin_rank+(dir.second*i);

                        if(isInBounds(next_square_file, next_square_rank) == false) break; //이 칸이 보드 경계를 벗어났다면

                        piece next_square = board[next_square_file][next_square_rank];
                        if(next_square.isEmpty()){ //칸이 비어있다면
                            continue;
                        }
                        else{ //칸이 비어있지 않는다면(기물이 존재한다면)
                            int final_destination_file = origin_file+(dir.first*(i+1));
                            int final_destination_rank = origin_rank+(dir.second*(i+1));

                            if(isInBounds(final_destination_file, final_destination_rank) == false) break; //뛰어넘을 칸이 보드 경계를 벗어났다면

                            piece final_destination = board[final_destination_file][final_destination_rank];
                            if(final_destination.isEmpty()){ //뛰어넘을 칸이 비어있다면
                                result.push_back(PGN(threatType::TAKEJUMP, origin_file, origin_rank, final_destination_file, final_destination_rank)); 
                                break;
                            }else if(final_destination.getColor() == current_piece.getColor()){ //뛰어넘을 칸에 아군이 있다면
                                break;
                            }else{ //뛰어넘을 칸에 적이 있다면
                                result.push_back(PGN(threatType::TAKEJUMP, origin_file, origin_rank, final_destination_file, final_destination_rank)); 
                                break;
                            }
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    return result;
}

void chessboard::updatePiece(PGN pgn)
{
    bool isLegal = false;
    auto fromSquare = pgn.getFromSquare();
    auto toSquare = pgn.getToSquare();
    threatType tT = pgn.getThreatType();

    std::vector<PGN> legal_move = calcLegalMovesInOnePiece(fromSquare.first, fromSquare.second);

    if(legal_move.empty()){
        return;
    }

    for(auto& move : legal_move){
        //std::cout << move.getFromSquare().first << move.getFromSquare().second << move.getToSquare().first << move.getToSquare().second << std::endl;
        if(pgn == move) {
            isLegal = true;
            break;
        }
    }
    if(!isLegal) {
        std::cout << "that pgn is illegal." << std::endl;
        return;
    }

    switch (tT)
    {
        case threatType::MOVE:
        case threatType::TAKEMOVE:
        case threatType::TAKEJUMP:
        case threatType::TAKE:
            movePiece(fromSquare.first, fromSquare.second, toSquare.first, toSquare.second);
            break;
        case threatType::CATCH:
            removePiece(toSquare.first, toSquare.second);
            break;
        case threatType::SHIFT:
            shiftPiece(fromSquare.first, fromSquare.second, toSquare.first, toSquare.second);
            break;
        default:
            break;
    }
}
