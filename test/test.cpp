#include <chess.hpp>

int main(){
    chessboard testboard;

    testboard.placePiece(colorType::WHITE, pieceType::KING, 4, 0); //wK@e1
    testboard.placePiece(colorType::BLACK, pieceType::KING, 4, 7); //bK@e8

    for(int i=0; i<7; i++) {
        testboard.placePiece(colorType::WHITE, pieceType::PWAN, 2, i); //프로모션 기물의 착수 칸에 따른 차등 스택 부여 기능 확인
        testboard.placePiece(colorType::BLACK, pieceType::PWAN, 6, 7-i); 
    } //wP@c1~c7, bR@g8~2

    std::cout << "e1에 있는 킹의 스턴 스택:" << testboard(4, 0).getStun() << std::endl;
    std::cout << "e8에 있는 킹의 스턴 스택:" << testboard(4, 7).getStun() << std::endl;
    std::cout << std::endl;
    for(int i=0; i<7; i++) std::cout << "c" << i+1 << "에 있는 폰의 스턴 스택:" << testboard(2, i).getStun() << std::endl; 
    std::cout << std::endl;
    for(int i=0; i<7; i++) std::cout << "g" << 8-i << "에 있는 폰의 스턴 스택:" << testboard(6, 7-i).getStun() << std::endl; 

    testboard.displayBoard();

    testboard(4, 0).setStun(0);
    testboard(4, 0).setMove(10);

    testboard.displayPieceInfo(4, 0);

    testboard.updatePiece(PGN(colorType::WHITE, threatType::TAKEMOVE, 4, 0, 4, 1));

    if(testboard(4, 0).isEmpty()) std::cout << "no piece in e1 square." << std::endl;
    if(!testboard(4, 1).isEmpty() && testboard(4, 1).getPieceType() == pieceType::KING) std::cout << "the king is in e2 square." << std::endl;

    testboard.displayBoard();

    testboard.placePiece(colorType::WHITE, pieceType::PWAN, 2, 7); //프로모션되는 칸에 착수 방지 확인
    
    return 0;
}
