# 변형 피스 추가 체크리스트

이제 이동/프로모션 규칙은 공유 PieceSpec 레지스트리에 있고, `piece`는 색/타입/스턴/무브/로열만 가집니다. 아래 순서로 추가하면 됩니다.

1) 열거형 추가
- 파일: engine/enum.hpp
- pieceType에 새 항목 추가 (기존 인덱스 의존 저장 데이터가 있으면 KING..TEMPESTROOK 순서 유지).

2) 규칙 정의 (이동, 프로모션 대상)
- 파일: engine/piece_spec.cpp
- makeSpec()에 새 pieceType 분기 추가.
- moveChunk(행동 종류, 방향, 최대 거리) 정의. 상단의 방향 상수 재사용 또는 추가.
- 프로모션 가능하면: spec.isPromotable = true; spec.promotePool 채우기; 색상별 spec.promotableSquares 채우기.

3) 스턴 기본값
- 파일: engine/piece_setting.cpp
- setupStunStack()에 새 기물의 기본 스턴 값 추가 (착수/위치 기반 설정에 사용).

4) 포켓 초기 수량
- 파일: engine/chess.hpp (chessboard 생성자)
- whitePocket/blackPocket 초기 배열에 새 기물 개수 반영 (pieceType 인덱스와 정렬).

5) 파이썬 바인딩
- 파일: python_bind/python_chess.cpp
- pybind11 enum export에 새 pieceType 추가 (Python에서 보이도록).

6) 어댑터/UI 심볼 매핑
- 파일: py/adapter.py (STR_TO_PIECE_TYPE, PIECE_TYPE_TO_STR)
- 파일: py/play.py (렌더링/라벨 커스텀 시)
- 입출력용으로 표시 가능한 심볼을 매핑.

7) 봇/휴리스틱
- 파일: py/bot.py
- PIECE_VALUES(네가맥스/웨이티드) 테이블에 적절한 가치 추가.

8) 테스트
- C++ 또는 Python 스모크 테스트로 새 기물 생성, 합법 수, 프로모션(있다면), 포켓 착수 규칙을 점검.

9) 빌드 & 실행
```bash
cd ~/바탕화면/project_bc_refectoring
cmake --build build -j
python3 py/play.py
```

메모
- isRoyal은 rule.md에 따라 동적이므로 스펙에 하드코딩하지 않습니다.
- 이동/프로모션 규칙은 레지스트리가 관리하고, `piece`는 동적 상태만 가집니다.
