#pragma once

// 기물 인덱스 (일반 기물 + 특수 기물 통합)
enum class pieceType{
    NONE=-1,
    KING,
    QUEEN,
    BISHOP,
    KNIGHT,
    ROOK,
    PWAN,
    AMAZON,
    GRASSHOPPER,
    KNIGHTRIDER,
    ARCHBISHOP,
    DABBABA,
    ALFIL,
    FERZ,
    CENTAUR,
    CAMEL,
    TEMPESTROOK
};

enum class colorType{
    NONE=-1,
    WHITE,
    BLACK
};


/*이 클래스는 각 기물이 행동반경 내에서 어떤 행동을 할 수 있는지를 정의한다.
catch:이 행마법은 행동반경에 존재하는 적 기물을 잡아낼 수 있으나 기물이 그 위치로 이동하지는 않는다.
take:이 행마법은 행동반경에 존재하는 적 기물이 있을 때"만" 잡아낸 다음 그 위치로 이동할 수 있다.
move:이 행마법은 행동반경 내에서 자유롭게 움직일 수 있으나, 적 기물이 행동반경에 존재할 경우 이동이 불가하다.
takemove:이 행마법은 가장 일반적인 행마법으로서, 행동반경 내에서 자유롭게 움직일 수 있고 적 기물이 행동반경 내에 존재할 경우 잡아내어 기 위치로 이동할 수 있다.
takejump:이 행마법은 행동반경에 기물이 존재할 때만 움직일 수 있으며, 그 기물을 뛰어넘은 다음 자신이 이동한 방향으로 1만큼 더 전진한 칸으로 이동한다. 도착 칸에 적 기물이 있으면 잡아낼 수 있다. ex)그래스호퍼
doubletakejump: 이 행마법은 행동반경에 적 기물이 존재할 때만 움직일 수 있으며, 그 기물을 잡아내고 뛰어넘은 다음 자신이 이동한 방향으로 1만큼 더 전진한 칸으로 이동한다. 도착 칸에 적 기물이 있으면 잡아낼 수 있다. ex)테스트룩
shift: 이 행마법은 행동반경 내에 다른 기물들과의 위치를 교환하는 행마법이다.
*/
enum class threatType{
    NONE=-1,
    CATCH,
    TAKE,
    MOVE,
    TAKEMOVE,
    TAKEJUMP,
    SHIFT
};



