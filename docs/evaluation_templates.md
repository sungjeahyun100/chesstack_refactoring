**Evaluation Class Template**

- **Purpose:** 설명과 예시 템플릿을 제공하여 `minimax` 기반 엔진에 새로운 평가 함수를 쉽게 추가하는 방법을 안내합니다.

- **Where to put it:** 새 평가 구현은 `src/bot_cpp/` 하위에 `.cpp`/.hpp로 추가하거나 기존 `minimax_gpt.cpp` 패턴을 복사해 사용하세요.

**Minimal Template (요약)**

- 만들기: `MyEval.cpp` 파일을 추가하고 내부에 `struct minimax_my_eval : public minimax { ... };` 형태로 구현합니다.
- 오버라이드: `eval_pos(const position& pos)`와 필요 시 `placement_score(const PGN&, colorType)`를 재정의합니다.
- 외부 래퍼: 헤더에 `class minimax_MyEval : public agent::bot` 스타일의 가벼운 래퍼를 두고 PIMPL 또는 직접 위임을 사용하세요.

**예시 코드 (요약된 의사코드)**

```cpp
// src/bot_cpp/minimax_my_eval.cpp
#include "agent.hpp"

namespace agent {

struct minimax_my_eval : public minimax {
    minimax_my_eval(position pos, colorType ct) : minimax(pos, ct) {}

    virtual int eval_pos(const position &pos) const override {
        // 1) material
        // 2) mobility
        // 3) placement
        // 4) threats
        // return centipawn int
    }

    virtual double placement_score(const PGN &pgn, colorType player) const override {
        // optional: override placement scoring used in gather_moves()
    }
};

// lightweight wrapper (optional)
struct minimax_MyEval::Impl { std::unique_ptr<minimax_my_eval> m; Impl(position p, colorType c):m(std::make_unique<minimax_my_eval>(p,c)){} };

minimax_MyEval::minimax_MyEval(position p, colorType c): impl(std::make_unique<Impl>(p,c)){}
minimax_MyEval::~minimax_MyEval() = default;
int minimax_MyEval::eval_pos(const position &pos) const { return impl->m->eval_pos(pos); }
PGN minimax_MyEval::getBestMove(position pos, int depth) { return impl->m->getBestMove(pos, depth); }

} // namespace agent
```

**노출 가능한 파라미터**

- `placement_sample` (size_t): 착수 샘플 수. 래퍼를 통해 `setPlacementSample(k)` 노출 권장.
- `iterative_deepening`, `use_aspiration`, `aspiration_window_base` 등: 탐색 동작 제어 플래그.
- 평가 가중치(예: material_weight, mobility_weight): 내부 멤버로 두고 생성자/세터로 노출하면 런타임 튜닝이 쉬움.

**테스트 & 벤치 사용법**

- `test/test_bot.cpp` 패턴을 복사해 새 봇을 인스턴스화하고 기존 벤치 루프에 추가하세요:

```cpp
agent::minimax_MyEval mybot(mutable_pos, colorType::WHITE);
mybot.setPlacementSample(10);
mybot.setIterativeDeepening(true);
PGN mv = mybot.getBestMove(mutable_pos, depth);
```

- 결과는 기존 CSV 출력 코드와 동일하게 기록하면 됩니다.

**권장 관례**

- 헤더에는 최소한의 공개 API만 노출(생성자, `getBestMove`, `setPlacementSample`, 기타 세터)
- 내부 구현은 `.cpp`에 숨겨 재컴파일 범위를 줄임(PIMPL 권장)
- `eval_pos`는 `position`의 읽기 전용 데이터만 사용하게 하고, `chessboard` 같은 임시 객체는 로컬 복사로 사용하세요.

**디버깅 팁**

- 새 평가를 도입할 때는 먼저 `depth=1`/`2`로 단명한 검색을 실행해 수 선택이 합리적인지 확인하세요.
- 성능 비교는 `nodes_searched` + 시간(ms) + 선택된 수(move)로 판단합니다.
- 수 정렬/비교 관련 UB를 피하려면 `MoveWrapper` 방식(사전 계산 후 정렬)을 사용하세요.

---

File: docs/evaluation_templates.md
