#pragma once
#include <chess.hpp>
#include <limits>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace agent{
    class bot{ //인터페이스
        public:
            virtual int eval_pos(const position& pos) const = 0; //평가 함수, -값이면 흑이 좋고, +값이면 백이 좋은 포지션
            virtual PGN getBestMove(position curr_pos, int depth) = 0; //최선수 찾기
            virtual std::vector<PGN> getBestLine(position curr_pos, int depth) = 0; //최선의 라인 찾기
    };

    class minimax : public bot{
        private:
            static constexpr int MAX_PLY = 64;

            colorType cT;
            position offset_board;
            chessboard simulate_board;

            // move ordering helpers
            std::vector<std::vector<PGN>> killers; // killers[depth] holds killer moves
            std::unordered_map<uint32_t, int> history; // history heuristic: move_key -> score

            uint32_t move_key(const PGN &m) const {
                auto f = m.getFromSquare(); auto t = m.getToSquare();
                uint32_t key = 0;
                key |= (static_cast<uint32_t>(f.first & 0xF) << 0);
                key |= (static_cast<uint32_t>(f.second & 0xF) << 4);
                key |= (static_cast<uint32_t>(t.first & 0xF) << 8);
                key |= (static_cast<uint32_t>(t.second & 0xF) << 12);
                key |= (static_cast<uint32_t>(static_cast<int>(m.getMoveType()) & 0xF) << 16);
                key |= (static_cast<uint32_t>(static_cast<int>(m.getPieceType()) & 0x1F) << 20);
                return key;
            }

            /*
             * 트랜스포지션 테이블(TT) 엔트리
             * - `key`: 위치의 Zobrist 해시(동등성 확인용)
             * - `value`: 저장된 평가값(저장 시점의 착수 측 관점으로 일관성 유지)
             * - `depth`: 이 엔트리가 저장될 때의 남은 탐색 깊이. 깊이가 큰 엔트리를 우선 보존함 (depth-prefer)
             * - `flag`: 값의 타입(0=EXACT, 1=LOWER, 2=UPPER) — 알파베타 윈도우 재사용에 사용
             * - `best`: 저장 시의 베스트 무브(PV)를 보관하여 이후 PV 힌트로 사용
             */
            struct TTEntry {
                uint64_t key = 0ULL;
                int value = 0;
                int depth = 0;
                uint8_t flag = 0; // 0=EXACT,1=LOWER,2=UPPER
                PGN best;
            };

            // 고정 크기 트랜스포지션 테이블(2의 거듭제곱 크기)
            // 구현 메모:
            // - 간단한 direct-mapped(직접 매핑) 테이블(index = zobrist & mask)을 사용하여
            //   상수 시간 접근과 캐시 친화성을 보장합니다.
            // - 대체 정책: depth-prefer. 저장 시 슬롯이 비어있거나 같은 키이거나 새 엔트리의
            //   깊이가 더 크거나 같으면 교체합니다.
            // - 가벼운 구현으로 예측 가능한 메모리 사용과 빠른 접근을 목표로 합니다.
            size_t tt_size_pow = 18; // 2^18 entries by default
            size_t tt_size = (1ULL<<tt_size_pow);
            size_t tt_mask = (tt_size - 1);
            std::vector<TTEntry> tt_table;
            void init_tt(size_t pow2 = 18);
            TTEntry* tt_probe(uint64_t key);
            void tt_store(uint64_t key, const TTEntry &entry);
            // Zobrist 해싱 상태
            // - `zobrist_pieces`와 `zobrist_pockets`는 평탄화된 벡터로 저장하여 색인 비용을 낮춥니다.
            // - `zobrist_pieces` 인덱스 레이아웃: ((pieceType*2 + color) * BOARDSIZE + file) * BOARDSIZE + rank
            // - `zobrist_pockets` 인덱스 레이아웃: (side * NUMBER_OF_PIECEKIND + pieceKind) * MAX_POCKET_COUNT + count
            // - `zobrist_side`는 착수 차례(side-to-move) 토글용 랜덤 값입니다.
            std::vector<uint64_t> zobrist_pieces; // size: NUMBER_OF_PIECEKIND * 2 * BOARDSIZE * BOARDSIZE
            std::vector<uint64_t> zobrist_pockets; // size: 2 * NUMBER_OF_PIECEKIND * MAX_POCKET_COUNT
            uint64_t zobrist_side[2];
            void init_zobrist();
            uint64_t compute_zobrist(const position &pos) const;
            uint64_t current_zobrist = 0ULL;
            void update_zobrist_for_move(uint64_t &h, const PGN &m, const chessboard &b, colorType player) const;

            int minimax_search(int depth, colorType player, int alpha, int beta, int ply, std::vector<PGN>& pv_out);
            std::vector<PGN> gather_moves(colorType player);
            int valueForBot() const; // 봇 관점의 현재 포지션 값 (simulate_board 이용)

            // quiescence search (captures & promotions)
            int quiescence(int alpha, int beta, int ply_depth, colorType player);
            std::vector<PGN> generate_captures_and_promotions(colorType player);

            // iterative deepening / PV (mutable control)
            std::vector<PGN> root_pv; // PV from last iterative deepening run

            // (moved to public section)

            // Helpers for ordering
            int static_exchange_eval(const PGN &m, const chessboard &b) const;
            void record_killer(int depth, const PGN &m);
            void record_history(const PGN &m, int depth);
            // 착수(placement) 가치 계산기: 특정 착수 PGN에 대해 플레이어 관점의 점수를 반환
            // 새로운 수식: 착수 가치 = (착수될 피스 자체의 가치) + TURN_VALUE * ((-1*(stun_on_place/3))^(거리))
            double placement_score(const PGN &pgn, colorType player) const;

        public:
            minimax(position pos, colorType ct) : cT(ct), offset_board(pos), killers(MAX_PLY) { history.reserve(1024); init_zobrist(); }

            // public diagnostics
            uint64_t nodes_searched = 0;

            // iterative deepening control + utility
            bool iterative_deepening = false; // enable iterative deepening
            bool use_aspiration = false; // enable aspiration windows
            int aspiration_window_base = 50; // initial aspiration window in centipawns
            // placement sampling: how many top placement moves to keep
            size_t placement_sample = 5;
            void setPlacementSample(size_t k) { placement_sample = k; }
            size_t getPlacementSample() const { return placement_sample; }

            // Accessors for iterative deepening / aspiration controls + nodes
            void setIterativeDeepening(bool v) { iterative_deepening = v; }
            bool getIterativeDeepening() const { return iterative_deepening; }
            void setUseAspiration(bool v) { use_aspiration = v; }
            bool getUseAspiration() const { return use_aspiration; }
            void setAspirationWindowBase(int val) { aspiration_window_base = val; }
            int getAspirationWindowBase() const { return aspiration_window_base; }
            void setNodeSearched(uint64_t val) {nodes_searched = val;}
            uint64_t getNodesSearched() const { return nodes_searched; }
            void resetNodesSearched() { nodes_searched = 0; }
            void reset_search_data();

            virtual int eval_pos(const position& pos) const override;
            virtual PGN getBestMove(position curr_pos, int depth) override;
            virtual std::vector<PGN> getBestLine(position curr_pos, int depth) override;
    };

    // Alternative minimax bot that uses the GPT-proposed evaluation function.
    // This class implements the `bot` interface and internally owns a
    // `minimax`-based implementation which overrides the evaluation function.
    class minimax_GPTproposed : public bot {
    public:
        minimax_GPTproposed(position pos, colorType ct);
        virtual ~minimax_GPTproposed();
        virtual int eval_pos(const position& pos) const override; // delegates to internal impl
        virtual PGN getBestMove(position curr_pos, int depth) override; // delegates to internal impl
        virtual std::vector<PGN> getBestLine(position curr_pos, int depth) override;
        // Control/inspection helpers forwarded to internal minimax implementation
        void setPlacementSample(size_t k);
        void reset_search_data();
        void setIterativeDeepening(bool v);
        void setUseAspiration(bool v);
        void setAspirationWindowBase(int val);
        void setNodesSearched(uint64_t val);
        uint64_t getNodesSearched() const;
    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
};

