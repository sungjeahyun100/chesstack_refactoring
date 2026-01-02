# 체스 봇 사용법

## 봇 종류

### SimpleBot
- 모든 합법적인 행동(이동, 드롭, 승계)을 동등하게 고려
- 무작위로 하나를 선택해서 실행

### WeightedBot
- 우선순위: 이동 > 드롭 > 승계
- 이동이 가능하면 이동을 먼저 시도
- 이동이 불가능할 때만 드롭 시도
- 둘 다 불가능할 때만 승계 시도

### NegamaxBot (고급)
- **네가맥스 알고리즘** 기반 AI
- 알파-베타 가지치기 포함
- 보드 상태를 평가하여 최선의 수 선택
- 기물 가치, 위치, 스택 상태 등을 종합적으로 고려
- 탐색 깊이 조정 가능 (기본값: 2)
- 더 느리지만 더 똑똑한 플레이

## UI에서 봇 사용하기

게임 실행:
```bash
cd py
python play.py
```

### 봇 조작 키:
- **B**: 검은색(black) 봇 켜기/끄기
- **Shift+B**: 흰색(white) 봇 켜기/끄기
- **N**: 봇 타입 전환 (weighted ↔ negamax)
- **R**: 게임 리셋 (봇도 함께 초기화됨)

봇이 활성화되면:
- 봇의 턴이 되면 자동으로 행동을 선택해서 실행
- 약 0.8초 간격으로 행동 (딜레이는 `UIState.bot_move_delay`에서 조정 가능)
- 화면 오른쪽 패널에 "Bot: [색상] ([타입])" 표시
- NegamaxBot은 평가 점수도 함께 표시

## 봇 테스트

기본 테스트 (WeightedBot vs WeightedBot):
```bash
cd py
python bot_test.py
```

네가맥스 봇 테스트 (NegamaxBot vs WeightedBot):
```bash
cd py
python bot_test.py negamax
```

## 봇 커스터마이징

### 네가맥스 봇 탐색 깊이 조정
`play.py`에서 봇 생성 시 depth 매개변수 변경:
```python
return NegamaxBot(engine, color, depth=3)  # 더 깊게 탐색 (더 느림, 더 똑똑함)
```

### 기물 가치 조정
`bot.py`의 `NegamaxBot.PIECE_VALUES` 딕셔너리 수정:
```python
PIECE_VALUES = {
    "K": 20000,
    "Q": 900,
    "R": 500,
    # ... 원하는 값으로 조정
}
```

### 평가 함수 개선
`NegamaxBot.evaluate_board()` 메서드를 수정하여:
- 킹 안전성 평가 추가
- 기물 이동성(mobility) 평가
- 포크, 핀 등의 전술 패턴 인식
- 엔드게임 테이블베이스 활용

### 새로운 봇 만들기
`bot.py`를 수정하거나 새 봇 클래스를 만들어서:
- Monte Carlo Tree Search (MCTS)
- 머신러닝 기반 평가 함수
- 오프닝 북(opening book) 통합

예시:
```python
from bot import NegamaxBot

class MyCustomBot(NegamaxBot):
    def evaluate_board(self):
        # 커스텀 평가 로직
        score = super().evaluate_board()
        # 추가 평가 요소
        return score
```

## 성능 고려사항

- **SimpleBot**: 매우 빠름, 약함
- **WeightedBot**: 빠름, 적당함
- **NegamaxBot (depth=2)**: 보통, 강함
- **NegamaxBot (depth=3)**: 느림, 매우 강함
- **NegamaxBot (depth=4+)**: 매우 느림, 엄청 강함

네가맥스 봇의 성능은 합법적 행동의 수에 따라 크게 달라집니다.
체스와 달리 드롭, 승계 등의 추가 행동이 있어 분기 계수(branching factor)가 더 높을 수 있습니다.
