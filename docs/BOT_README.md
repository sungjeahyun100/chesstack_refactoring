# 체스 봇 사용법 (최신)

## 지원 봇 타입
- **Minimax**: 기본 탐색 봇 (C++ `Minimax`).
- **Minimax-GPT**: GPT 제안 버전 (`MinimaxGPT`).

UI에서 노출되는 이름은 `my_propose`, `GPTproposed`이며 시작/인게임 Bot Type 버튼으로 선택합니다.

## 실행 및 흐름
```bash
cd py
python play.py
```
1) 모드 선택: Friend / Bot / Analysis.
2) Bot/Analysis 선택 시: 색상(플레이어 색) → Bot Type 선택 메뉴가 순서대로 뜹니다. (봇은 반대 색을 잡거나 분석용으로 사용)
3) 인게임 상단 오른쪽의 **Bot Type** 버튼으로 언제든 타입을 다시 고를 수 있습니다.
4) Friend 모드에서는 봇·분석 패널이 숨겨지고 창 높이가 줄어듭니다.

## 조작(마우스 중심)
- 말 이동: 말 클릭 → 목표 칸 클릭.
- 드롭: 오른쪽 패널 포켓 클릭 → 보드 칸 클릭.
- 더블클릭: 액션 선택 메뉴 오픈 (succession, stun, 로얄이면 disguise 추가). disguise를 눌러야 위장 선택 창이 뜹니다.
- 프로모션/위장 선택: 패널 상단의 오버레이에서 선택.
- 메뉴 버튼: 우측 상단 MENU로 초기 모드 선택 화면 복귀.
- 단축키: `R` 리셋, `Q/Esc` 종료, `TAB`/화살표/SPACE/ENTER는 포커스·모드 순환용(선택적).

## 분석 패널 (Bot/Analysis 모드)
- 우측 하단에 분석 점수, PV, 마지막 봇 수가 표시됩니다.
- 봇 생성 시 `ui.depth`가 전달되며, Bot Type 버튼으로 타입을 바꿔도 새 봇이 재생성됩니다.

## 커스터마이징
- 깊이 조정: `UIState.depth` 초기값 변경 또는 `create_bot` 호출 전후 값 수정.
- 배치 샘플 크기: `UIState.placement_sample` → 봇 생성 시 `_bot.setPlacementSample`로 반영.
- 새 봇 추가: `py/bot.py`에 래퍼를 만들고, `ui/bot_manager.py`의 `create_bot`과 `play.py`의 `BOT_TYPES`에 이름을 추가하면 선택 메뉴에 노출됩니다.
