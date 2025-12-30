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

std::vector<PGN> chessboard::calcLegalMovesInOnePiece(colorType cT, int file, int rank, bool calc_potential)
{
    std::vector<PGN> result;
    result.clear();

    if(board[file][rank].isEmpty()){
        //std::cout << "there has no pieces." << std::endl;
        return std::vector<PGN>();
    }

    auto current_piece = board[file][rank];
    auto pieceColor = current_piece.getColor();

    if(!calc_potential && current_piece.getStun() > 0){
        //std::cout << "that piece is stunned." << std::endl;
        return std::vector<PGN>();
    }

    if(!calc_potential && current_piece.getMove() == 0){
        //std::cout << "that piece has no move stack." << std::endl;
        return std::vector<PGN>();
    }

    if(current_piece.getColor() != cT){
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
                            result.push_back(PGN(cT, threatType::CATCH, file, rank, next_square_file, next_square_rank)); // 칸에 적 기물이 있다면
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
                            result.push_back(PGN(cT, threatType::TAKEMOVE, file, rank, next_square_file, next_square_rank)); //칸이 비어있다면
                            continue;
                        }
                        if(next_square.getColor() == current_piece.getColor()) break; //칸에 아군이 있다면
                        if(next_square.getColor() != current_piece.getColor()) {
                            result.push_back(PGN(cT, threatType::TAKEMOVE, file, rank, next_square_file, next_square_rank)); // 칸에 적 기물이 있다면
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
                            result.push_back(PGN(cT, threatType::MOVE, file, rank, next_square_file, next_square_rank)); //칸이 비어있다면
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
                            result.push_back(PGN(cT, threatType::SHIFT, file, rank, next_square_file, next_square_rank));//칸에 아군이 있다면
                            break;
                        }
                        if(next_square.isEmpty()){
                            continue; //칸이 비어있다면
                        }
                        if(next_square.getColor() != current_piece.getColor()) {
                            result.push_back(PGN(cT, threatType::SHIFT, file, rank, next_square_file, next_square_rank));// 칸에 적 기물이 있다면
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
                            result.push_back(PGN(cT, threatType::TAKE, file, rank, next_square_file, next_square_rank));
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
                                result.push_back(PGN(cT, threatType::TAKEJUMP, file, rank, final_destination_file, final_destination_rank)); 
                                break;
                            }else if(final_destination.getColor() == current_piece.getColor()){ //뛰어넘을 칸에 아군이 있다면
                                break;
                            }else{ //뛰어넘을 칸에 적이 있다면
                                result.push_back(PGN(cT, threatType::TAKEJUMP, file, rank, final_destination_file, final_destination_rank)); 
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

    if(current_piece.getIsPromotable()){ 
        //일단 당장은 잘 작동할 거다. 애초에 프로모션 가능한 기물들은 약한 기물들 뿐이라 PGN이 그렇게 많지도 않을 거라서 성능 악화는 없을 것이다. 근데 그런 게 생겨버리면 그건 이제 그때 가서 생각하자.
        std::vector<PGN> promote;
        auto promotable_square = current_piece.getPromotableSquare();
        auto promotable_pieces = current_piece.getPromotePool();
        for(int i=0; i<result.size(); i++){
            for(auto& p_sq : promotable_square){
                if(result[i].getToSquare() == p_sq){
                    for(auto& promotable_piece : promotable_pieces){
                        promote.push_back(PGN(cT, result[i].getThreatType(), file, rank, result[i].getToSquare().first, result[i].getToSquare().second, promotable_piece));
                    }
                    result[i] = PGN();
                }
            }
        }

        result.erase(std::remove(result.begin(), result.end(), PGN()), result.end());

        result.insert(result.end(), promote.begin(), promote.end());
    }

    return result;
}

std::vector<PGN> chessboard::calcLegalPlacePiece(colorType cT)
{
    std::vector<PGN> result;

    auto collectPlacements = [&](colorType color, const std::array<int, NUMBER_OF_PIECEKIND>& pocket){
        for(int idx = 0; idx < NUMBER_OF_PIECEKIND; ++idx){
            if(pocket[idx] <= 0) continue; // 해당 기물이 포켓에 없음

            pieceType pT = static_cast<pieceType>(idx);
            piece candidate(color, pT);

            for(int file = 0; file < BOARDSIZE; ++file){
                for(int rank = 0; rank < BOARDSIZE; ++rank){
                    if(!board[file][rank].isEmpty()) continue; // 빈 칸에만 착수 가능

                    // 프로모션 가능 기물은 프로모션 칸에 직접 착수할 수 없음
                    if(candidate.getIsPromotable()){
                        bool on_promotable_square = false;
                        for(const auto& promotable_sq : candidate.getPromotableSquare()){
                            if(promotable_sq.first == file && promotable_sq.second == rank){
                                on_promotable_square = true;
                                break;
                            }
                        }
                        if(on_promotable_square) continue;
                    }

                    result.push_back(PGN(color, file, rank, pT));
                }
            }
        }
    };

    if(cT == colorType::WHITE) collectPlacements(colorType::WHITE, whitePocket);
    else if(cT == colorType::BLACK) collectPlacements(colorType::BLACK, blackPocket);
    else return std::vector<PGN>();

    return result;
}

std::vector<PGN> chessboard::calcLegalSuccesion(colorType cT)
{
    std::vector<PGN> result;

    for(int file = 0; file < BOARDSIZE; ++file){
        for(int rank = 0; rank < BOARDSIZE; ++rank){
            const piece& target = board[file][rank];
            if(target.isEmpty()) continue;
            if(target.getIsRoyal()) continue; // 이미 로얄인 경우 제외
            if(target.getColor() != cT) continue;

            result.push_back(PGN(cT, file, rank, moveType::SUCCESION));
        }
    }

    return result;
}

void chessboard::updatePiece(PGN pgn)
{
    auto mT = pgn.getMoveType();
    bool isLegal = false;
    auto fromSquare = pgn.getFromSquare();
    auto toSquare = pgn.getToSquare();
    threatType tT = pgn.getThreatType();
    auto pT = pgn.getPieceType();
    auto cT = pgn.getColorType();

    std::vector<PGN> legal_move;
    if(mT == moveType::MOVE || mT == moveType::PROMOTE) legal_move = calcLegalMovesInOnePiece(turn_right, fromSquare.first, fromSquare.second, false);
    else if(mT == moveType::ADD) legal_move = calcLegalPlacePiece(turn_right);
    else if(mT == moveType::SUCCESION) legal_move = calcLegalSuccesion(turn_right);

    if(legal_move.empty()){
        return;
    }

    for(auto& move : legal_move){
        if(pgn == move) {
            isLegal = true;
            break;
        }
    }

    if(!isLegal) {
        std::cout << "that pgn is illegal." << std::endl;
        return;
    }

    // 스냅샷 저장 (undo를 위해 현재 상태를 position으로 저장)
    snapshots.push_back(getPosition());

    if(mT == moveType::MOVE){
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
    }else if(mT == moveType::ADD){
        placePiece(cT, pT, fromSquare.first, fromSquare.second);
    }else if(mT == moveType::SUCCESION){
        succesionPiece(fromSquare.first, fromSquare.second);
    }else if(mT == moveType::PROMOTE){
        cT = board[fromSquare.first][fromSquare.second].getColor();
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
        promotePiece(cT, toSquare.first, toSquare.second, pT);
    }

    log.push_back(pgn);
    // advance turn after a successful move
    turn_right = (turn_right == colorType::WHITE) ? colorType::BLACK : colorType::WHITE;
}

void chessboard::pieceStackControllByColor(colorType cT, int d_stun, int d_move)
{
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            if(board[i][j].getColor() == cT){
                board[i][j].addStun(d_stun);
                board[i][j].addMove(d_move);
            }
        }
    }
}

void chessboard::undoBoard(){
    if(snapshots.empty()){
        // fallback: if no snapshot, fallback to popping last log entry
        if(!log.empty()) log.pop_back();
        // also flip turn since we popped a move
        turn_right = (turn_right == colorType::WHITE) ? colorType::BLACK : colorType::WHITE;
        return;
    }

    position prev = snapshots.back();
    snapshots.pop_back();

    // restore full position
    setPosition(prev);
}