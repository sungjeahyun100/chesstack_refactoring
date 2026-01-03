#include "agent.hpp"
#include <cmath>
#include <random>
#include <limits>

namespace agent{

    // 단순한 기물 기반 평가
    static int piece_value(pieceType pt) {
        switch(pt){
            case pieceType::KING: return 400;
            case pieceType::QUEEN: return 900;
            case pieceType::ROOK: return 500;
            case pieceType::BISHOP: return 330;
            case pieceType::KNIGHT: return 320;
            case pieceType::PWAN: return 100;
            case pieceType::AMAZON: return 1400;        // 아마존
            case pieceType::GRASSHOPPER: return 280;    // 그래스호퍼
            case pieceType::KNIGHTRIDER: return 650;   // 나이트라이더
            case pieceType::ARCHBISHOP: return 800;    // 아치비숍 비숍+나이트
            case pieceType::DABBABA: return 250;       // 다바바
            case pieceType::ALFIL: return 250;         // 알필
            case pieceType::FERZ: return 150;          // 페르츠
            case pieceType::CENTAUR: return 700;       // 센타우르
            case pieceType::CAMEL: return 450;         // 캐멀
            case pieceType::TEMPESTROOK: return 700;   // 테스트룩
            default: return 0;
        }
    }

    int minimax::eval_pos(const position& pos) const {
        const double TURN_VALUE = 0.3;
        const int STUN_ON_ROYAL_DEATH = 3; // 착수(놓기) 시에도 '스턴'으로 사용됨

        // 이동 가능한 수 계산을 위해 position으로부터 보조 체스보드 생성
        position pos_copy = pos; // make a mutable copy
        chessboard tmp(pos_copy);

        double score = 0.0;

        // 각 색상의 기본 기물 값 합과 로열(왕 등) 개수 사전 계산
        double sum_base_white = 0.0;
        double sum_base_black = 0.0;
        int royal_count_white = 0;
        int royal_count_black = 0;

        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = pos.board[f][r];
                if(p.getPieceType() == pieceType::NONE) continue;
                int bv = piece_value(p.getPieceType());
                if(p.getColor() == colorType::WHITE) sum_base_white += bv;
                else sum_base_black += bv;
                if(p.getIsRoyal()){
                    if(p.getColor() == colorType::WHITE) ++royal_count_white;
                    else ++royal_count_black;
                }
            }
        }

        // 보드에 놓인 기물 평가
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = pos.board[f][r];
                if(p.getPieceType() == pieceType::NONE) continue;

                double base = static_cast<double>(piece_value(p.getPieceType()));
                double factor = 1.0; // 승리 판정 함수를 사용하므로 로얄 가중치는 제거

                // 이 기물이 가질 수 있는 잠재적 행동(수) 개수
                int num_actions = 0;
                auto moves_for_piece = tmp.calcLegalMovesInOnePiece(p.getColor(), f, r, true);
                num_actions += static_cast<int>(moves_for_piece.size());

                double placed_value = factor * base
                                      + (TURN_VALUE * num_actions * p.getMove())
                                      + (-TURN_VALUE * p.getStun());

                if(p.getColor() == colorType::WHITE) score += placed_value;
                else score -= placed_value;
            }
        }

        // 포켓(보유) 기물 평가
        for(int i=0;i<NUMBER_OF_PIECEKIND;++i){
            pieceType pt = static_cast<pieceType>(i);
            double base = static_cast<double>(piece_value(pt));
            double pocket_single = base + (-TURN_VALUE * STUN_ON_ROYAL_DEATH);
            if(pos.whitePocket[i] > 0) score += pocket_single * pos.whitePocket[i];
            if(pos.blackPocket[i] > 0) score -= pocket_single * pos.blackPocket[i];
        }

        return static_cast<int>(std::round(score));
    }

    int minimax::valueForBot() const {
        auto pos = simulate_board.getPosition();
        int v = eval_pos(pos);
        return (cT == colorType::WHITE) ? v : -v;
    }

    void minimax::reset_search_data(){
        for(auto &k : killers) k.clear();
        history.clear();
        root_pv.clear();
        nodes_searched = 0;
        // TT는 검색 간에 남겨두면 이후 러닝이 비정상적으로 빨라질 수 있으니 초기화
        tt_table.assign(tt_size, TTEntry{});
        current_zobrist = 0ULL;
    }

    int minimax::static_exchange_eval(const PGN &m, const chessboard &b) const {
        // 단순화된 SEE: 만약 캡처라면 값 = 희생자 가치 - 공격자 가치
        if(m.getThreatType() == threatType::NONE) return 0;
        auto to = m.getToSquare();
        int tf = to.first, tr = to.second;
        const piece &victim = b.at(tf, tr);
        if(victim.getPieceType() == pieceType::NONE) {
            // 빈 칸으로 이동하거나 캡처가 아닌 위협일 수 있음
            if(m.getMoveType() == moveType::PROMOTE){
                // 승격 이득: 승격한 기물 가치 - 기본 폰 가치
                int promoted = piece_value(m.getPieceType());
                int pawnv = piece_value(pieceType::PWAN);
                return promoted - pawnv;
            }
            return 0;
        }
        // 공격자: 출발 칸
        auto from = m.getFromSquare();
        int ff = from.first, fr = from.second;
        const piece &att = b.at(ff, fr);
        int val_victim = piece_value(victim.getPieceType());
        int val_att = piece_value(att.getPieceType());
        int score = val_victim - val_att;
        // 낮은 가치의 공격자로 캡처하는 것을 선호하도록 공격자 패널티를 축소
        return score;
    }

    void minimax::init_zobrist(){
        // sizes
        const size_t PIECE_SLOTS = static_cast<size_t>(NUMBER_OF_PIECEKIND) * 2 * BOARDSIZE * BOARDSIZE;
        const int MAX_POCKET_COUNT = 32;
        const size_t POCKET_SLOTS = 2 * static_cast<size_t>(NUMBER_OF_PIECEKIND) * MAX_POCKET_COUNT;

        zobrist_pieces.assign(PIECE_SLOTS, 0ULL);
        zobrist_pockets.assign(POCKET_SLOTS, 0ULL);

        std::mt19937_64 rng(0x9e3779b97f4a7c15ULL);
        for(size_t i=0;i<PIECE_SLOTS;++i) zobrist_pieces[i] = rng();
        for(size_t i=0;i<POCKET_SLOTS;++i) zobrist_pockets[i] = rng();
        zobrist_side[0] = rng(); zobrist_side[1] = rng();
        // 트랜스포지션 테이블 초기화
        // 기본값으로 2^18 엔트리를 사용합니다. 실험/튜닝을 위해 `init_tt`로 크기 조정 가능합니다.
        // direct-mapped(비트마스크 인덱싱) 구조를 사용하여 인덱싱 비용을 낮췄습니다.
        init_tt(18);
    }

    uint64_t minimax::compute_zobrist(const position &pos) const {
        uint64_t h = 0ULL;
        // pieces
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = pos.board[f][r];
                if(p.getPieceType() == pieceType::NONE) continue;
                int pt = static_cast<int>(p.getPieceType());
                int color = (p.getColor() == colorType::WHITE) ? 0 : 1;
                size_t idx = ((pt * 2 + color) * BOARDSIZE + f) * BOARDSIZE + r;
                if(idx < zobrist_pieces.size()) h ^= zobrist_pieces[idx];
            }
        }
        // pockets
        const auto &wp = pos.whitePocket;
        const auto &bp = pos.blackPocket;
        const int MAX_POCKET_COUNT = 32;
        for(int i=0;i<NUMBER_OF_PIECEKIND;++i){
            int cnt = wp[i];
            if(cnt < MAX_POCKET_COUNT){
                size_t idx = (0 * NUMBER_OF_PIECEKIND + i) * MAX_POCKET_COUNT + cnt;
                if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
            }
            cnt = bp[i];
            if(cnt < MAX_POCKET_COUNT){
                size_t idx = (1 * NUMBER_OF_PIECEKIND + i) * MAX_POCKET_COUNT + cnt;
                if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
            }
        }
        return h;
    }

    /*
     * init_tt
     * 검색에서 사용하는 고정 크기 트랜스포지션 테이블을 할당하고 초기화합니다.
     * 2의 거듭제곱 크기를 사용하면 비트마스크(`key & mask`)로 인덱스를 빠르게 계산할 수 있습니다.
     * 엔트리는 0으로 초기화되며 `key==0`을 빈 슬롯 표시로 사용합니다(실제 해시가 0이 될 확률은 매우 낮음).
     */

    void minimax::init_tt(size_t pow2){
        tt_size_pow = pow2;
        tt_size = (1ULL<<tt_size_pow);
        tt_mask = tt_size - 1;
        tt_table.clear();
        tt_table.resize(tt_size);
    }

    // probe TT: return pointer to entry if key matches; else nullptr
    minimax::TTEntry* minimax::tt_probe(uint64_t key){
        size_t idx = static_cast<size_t>(key & tt_mask);
        TTEntry &e = tt_table[idx];
        if(e.key == key) return &e;
        return nullptr;
    }

    // store TT with depth-prefer replacement
    void minimax::tt_store(uint64_t key, const TTEntry &entry){
        size_t idx = static_cast<size_t>(key & tt_mask);
        TTEntry &e = tt_table[idx];
        if(e.key == 0ULL){
            e = entry; e.key = key; return;
        }
        // replace if same key, or deeper depth, else keep existing
        if(e.key == key || entry.depth >= e.depth){
            e = entry; e.key = key; return;
        }
        // otherwise, keep existing
    }

    /*
     * update_zobrist_for_move
     * 단일 PGN 수의 증분 해시 효과를 현재 해시 `h`에 적용합니다.
     * 사용 패턴: 수를 적용하기 전에 호출 -> `updatePiece(m)`로 보드 적용 -> 검색 후 `undoBoard()`로 복구 ->
     * 다시 이 함수를 호출하면 xor의 역원 성질로 원래 해시가 복원됩니다. 따라서 전체 해시를 다시 계산하지 않고
     * 빠르게 해시를 갱신/복원할 수 있습니다.
     *
     * 유의사항:
     * - 이 함수는 보드 `b`의 현재 상태(수 적용 전)를 읽습니다. 호출 순서를 지켜야 해시가 올바르게 유지됩니다.
     * - `ADD` 수는 포켓 카운트 변화를 처리하고 착수된 기물을 xor 합니다.
     * - `MOVE`/`PROMOTE`는 출발칸의 공격자 xor 제거, 도착칸의 희생자 xor 제거, 도착칸에 들어갈 기물 xor 추가 순으로 처리합니다.
     */

    // helper: xor piece slot for a given board square into hash
    static inline void xor_piece_slot(uint64_t &h, const std::vector<uint64_t> &zobrist_pieces, int f, int r, const piece &p){
        if(p.getPieceType() == pieceType::NONE) return;
        int pt = static_cast<int>(p.getPieceType());
        int color = (p.getColor() == colorType::WHITE) ? 0 : 1;
        size_t idx = ((pt * 2 + color) * BOARDSIZE + f) * BOARDSIZE + r;
        if(idx < zobrist_pieces.size()) h ^= zobrist_pieces[idx];
    }

    void minimax::update_zobrist_for_move(uint64_t &h, const PGN &m, const chessboard &b, colorType player) const {
        const int MAX_POCKET_COUNT = 32;
        auto mT = m.getMoveType();
        auto from = m.getFromSquare();
        auto to = m.getToSquare();
        auto pT = m.getPieceType();
        auto cT = m.getColorType();

        // flip side: remove current side constant and add opponent side constant
        int curSide = (player == colorType::WHITE) ? 0 : 1;
        int oppSide = (player == colorType::WHITE) ? 1 : 0;
        h ^= zobrist_side[curSide]; // remove current
        h ^= zobrist_side[oppSide]; // add opponent

        // snapshot pockets for potential capture handling
        const auto &wp_before = b.getWhitePocket();
        const auto &bp_before = b.getBlackPocket();

        if(mT == moveType::MOVE || mT == moveType::PROMOTE){
            // attacker at from
            const piece &att = b.at(from.first, from.second);
            xor_piece_slot(h, zobrist_pieces, from.first, from.second, att); // remove attacker from from

            // victim at to (if any) -- may be empty
            const piece &vict = b.at(to.first, to.second);
            if(!vict.isEmpty()) {
                xor_piece_slot(h, zobrist_pieces, to.first, to.second, vict); // remove victim
                // if victim will be captured, the capturer's pocket increases by 1 -> update pocket hash
                pieceType vpt = vict.getPieceType();
                if(vpt != pieceType::NONE){
                    if(player == colorType::WHITE){
                        int oldc = wp_before[static_cast<int>(vpt)];
                        if(oldc < MAX_POCKET_COUNT){
                            size_t idx = (0 * NUMBER_OF_PIECEKIND + static_cast<int>(vpt)) * MAX_POCKET_COUNT + oldc;
                            if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                        }
                        int newc = std::min(MAX_POCKET_COUNT-1, oldc + 1);
                        if(newc < MAX_POCKET_COUNT){
                            size_t idx = (0 * NUMBER_OF_PIECEKIND + static_cast<int>(vpt)) * MAX_POCKET_COUNT + newc;
                            if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                        }
                    } else {
                        int oldc = bp_before[static_cast<int>(vpt)];
                        if(oldc < MAX_POCKET_COUNT){
                            size_t idx = (1 * NUMBER_OF_PIECEKIND + static_cast<int>(vpt)) * MAX_POCKET_COUNT + oldc;
                            if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                        }
                        int newc = std::min(MAX_POCKET_COUNT-1, oldc + 1);
                        if(newc < MAX_POCKET_COUNT){
                            size_t idx = (1 * NUMBER_OF_PIECEKIND + static_cast<int>(vpt)) * MAX_POCKET_COUNT + newc;
                            if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                        }
                    }
                }
            }

            // insert attacker at to (if promotion, use promoted type)
            if(mT == moveType::PROMOTE){
                // promoted piece type is in m.getPieceType()
                piece promoted(att.getColor(), pT);
                xor_piece_slot(h, zobrist_pieces, to.first, to.second, promoted);
            } else {
                xor_piece_slot(h, zobrist_pieces, to.first, to.second, att);
            }
        } else if(mT == moveType::ADD){
            // pocket count change and placed piece
            int ptidx = static_cast<int>(pT);
            const auto &wp = b.getWhitePocket();
            const auto &bp = b.getBlackPocket();
            if(cT == colorType::WHITE){
                int oldc = wp[ptidx];
                if(oldc < MAX_POCKET_COUNT) {
                    size_t idx = (0 * NUMBER_OF_PIECEKIND + ptidx) * MAX_POCKET_COUNT + oldc;
                    if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                }
                int newc = std::max(0, oldc-1);
                if(newc < MAX_POCKET_COUNT){
                    size_t idx = (0 * NUMBER_OF_PIECEKIND + ptidx) * MAX_POCKET_COUNT + newc;
                    if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                }
            } else {
                int oldc = bp[ptidx];
                if(oldc < MAX_POCKET_COUNT) {
                    size_t idx = (1 * NUMBER_OF_PIECEKIND + ptidx) * MAX_POCKET_COUNT + oldc;
                    if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                }
                int newc = std::max(0, oldc-1);
                if(newc < MAX_POCKET_COUNT){
                    size_t idx = (1 * NUMBER_OF_PIECEKIND + ptidx) * MAX_POCKET_COUNT + newc;
                    if(idx < zobrist_pockets.size()) h ^= zobrist_pockets[idx];
                }
            }
            // placed piece at from coords
            piece placed(cT, pT);
            xor_piece_slot(h, zobrist_pieces, from.first, from.second, placed);
        } else if(mT == moveType::SUCCESION){
            // no zobrist effect (we don't track royalty)
        } else if(mT == moveType::DISGUISE){
            // disguise: same square, same color, piece type changes
            const piece &oldp = b.at(from.first, from.second);
            xor_piece_slot(h, zobrist_pieces, from.first, from.second, oldp); // remove old type
            piece disguised(oldp.getColor(), pT);
            xor_piece_slot(h, zobrist_pieces, from.first, from.second, disguised); // add new type
        }
    }

    void minimax::record_killer(int depth, const PGN &m){
        if(depth < 0 || depth >= static_cast<int>(killers.size())) return;
        auto &vec = killers[depth];
        // 최대 2개의 킬러 수 저장
        if(vec.empty()) { vec.push_back(m); return; }
        if(vec.size() == 1){ if(!(vec[0] == m)) vec.push_back(m); return; }
        // 크기==2
        if(vec[0] == m || vec[1] == m) return; // 이미 존재함
        // 두 번째 엔트리 교체
        vec[1] = m;
    }

    void minimax::record_history(const PGN &m, int depth){
        uint32_t key = move_key(m);
        // reward by depth squared so shallower cutoffs get smaller reward
        history[key] += (depth * depth + 1);
    }

    // 착수(placement) PGN의 가치 계산 (플레이어 관점). gather_moves에서 사용됨.
    // 새 수식: 착수 가치 = (착수될 피스 자체의 가치) + TURN_VALUE * ((-1 * (stun_on_place / 3)) ^ 거리)
    // 구현 주의: 음수 밑수의 비정수 제곱은 실수가 아니므로 안전하게 음수 밑수일 때는 절댓값의 제곱에 음의 부호를 붙여 반환합니다.
    double minimax::placement_score(const PGN &pgn, colorType player) const {
        const double TURN_VALUE = 0.3;
        const double STUN_ON_PLACE = 3.0;
        auto from = pgn.getFromSquare();
        int pf = from.first, pr = from.second;
        pieceType pt = pgn.getPieceType();

        double base = static_cast<double>(piece_value(pt));

        double dx = 4.5 - static_cast<double>(pf);
        double dy = 4.5 - static_cast<double>(pr);
        double dist = std::sqrt(dx*dx + dy*dy);

        double base_ratio = -1.0 * (STUN_ON_PLACE / 3.0);
        double pow_val;
        if(base_ratio < 0.0){
            // 음수 밑수이고 거리(dist)가 실수일 수 있으므로 실수 결과를 얻기 위해 절댓값으로 계산하고 음의 부호를 붙임
            pow_val = -std::pow(std::abs(base_ratio), dist);
        } else {
            pow_val = std::pow(base_ratio, dist);
        }

        double modifier = TURN_VALUE * pow_val;

        double placement_value = base + modifier;
        return (player == colorType::WHITE) ? placement_value : -placement_value;
    }

    std::vector<PGN> minimax::gather_moves(colorType player){
        std::vector<PGN> res;

        // 착수(드롭) 가능한 수 먼저 수집 (포켓이 비어 있으면 건너뜀)
        auto placements = simulate_board.calcLegalPlacePiece(player);
        if(!placements.empty()){
            int log_size = simulate_board.getLogSize();
            bool custom_pos = simulate_board.getThisPositionIsCustom();
            bool restrict_to_king = (!custom_pos && log_size < 2); // 기본 포지션 초반에는 킹 착수만 허용
            if(restrict_to_king){
                std::vector<PGN> filtered;
                filtered.reserve(placements.size());
                for(const auto &pgn : placements){
                    if(pgn.getPieceType() == pieceType::KING) filtered.push_back(pgn);
                }
                placements = std::move(filtered);
            }

            // 착수(placements) 우선순위 계산 및 샘플링
            std::vector<std::pair<double, PGN>> scored;
            scored.reserve(placements.size());
            for(const auto &pgn : placements){
                if(restrict_to_king && pgn.getPieceType() != pieceType::KING) continue; // 초기 수면 킹 착수만 허용
                double score = placement_score(pgn, player);
                scored.emplace_back(score, pgn);
            }

            // 점수 내림차순 정렬 (플레이어에 유리한 순서)
            std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b){ return a.first > b.first; });

            // 샘플링: 상위 K개만 사용(너무 많은 착수로 브랜치 폭 증가 방지)
            size_t take = std::min(scored.size(), static_cast<size_t>(placement_sample));
            for(size_t i=0;i<take;++i) res.push_back(scored[i].second);
        }

        // 전부의 칸에서 합법 수를 수집 (보드 API에 맞춰 호출)
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                if(simulate_board.at(f, r).isEmpty()) continue;
                // 수집하려는 기물은 player의 소유여야 함
                if(simulate_board.at(f, r).getColor() != player) continue;
                auto moves = simulate_board.calcLegalMovesInOnePiece(player, f, r, false); //이동 & 승격
                if(!moves.empty()){
                    for(auto &m : moves){
                        if(m.getColorType() == player) res.push_back(m);
                    }
                }
                // 계승: 법적 계승 수만 추가 (calcLegalSuccesion 사용)
            }
        }

        // collect legal succesion PGNs and append those matching player
        auto succs = simulate_board.calcLegalSuccesion(player);
        if(!succs.empty()){
            for(const auto &pgn : succs){
                res.push_back(pgn);
            }
        }

        // collect legal disguise PGNs and append
        auto disguises = simulate_board.calcLegalDisguise(player);
        if(!disguises.empty()){
            for(const auto &pgn : disguises){
                res.push_back(pgn);
            }
        }

        return res;
    }

    int minimax::minimax_search(int depth, colorType player, int alpha, int beta, int ply, std::vector<PGN>& pv_out)
    {
        nodes_searched++;
        if (depth == 0) return quiescence(alpha, beta, 0, player);

        // Transposition table lookup
        // use the incremental current_zobrist which is kept in sync by update_zobrist_for_move
        uint64_t h = current_zobrist;
        int original_alpha = alpha;
        int original_beta = beta;
        {
            TTEntry *te_ptr = tt_probe(h);
            if(te_ptr != nullptr){
                const TTEntry &te = *te_ptr;
                if(te.depth >= depth){
                    if(te.flag == 0) {
                        if(te.best.getMoveType() != moveType::NONE){
                            pv_out.clear();
                            pv_out.push_back(te.best);
                        }
                        return te.value; // exact
                    }
                    if(te.flag == 1) alpha = std::max(alpha, te.value); // lowerbound
                    else if(te.flag == 2) beta = std::min(beta, te.value); // upperbound
                    if(alpha >= beta) return te.value;
                }
            }
        }

        auto moves = gather_moves(player);
        // filter out explicit NONE moves (safety)
        moves.erase(std::remove_if(moves.begin(), moves.end(), [](const PGN &m){ return m.getMoveType() == moveType::NONE; }), moves.end());
        // diagnostic: print basic info to find corrupt entries before sorting
        if (moves.empty()) return valueForBot();

        // 수순 정렬: PV 우선(있을 경우), 캡처/승격(SEE), 킬러 수, 히스토리 휴리스틱
        PGN pv_move;
        bool have_pv_move = false;
        if(iterative_deepening && ply < static_cast<int>(root_pv.size())){
            pv_move = root_pv[ply];
            have_pv_move = true;
        }

        // 안정성: 정렬 중 comparator가 외부 상태(simulate_board, killers, history)를 직접 조회하면
        // 재귀/이동으로 인한 댕글링 참조/타이밍 문제로 메모리 손상이 발생할 수 있다.
        // 따라서 먼저 각 수에 대해 SEE, 히스토리, 킬러 플래그를 계산해 래핑한 뒤 정렬한다.
        struct MoveWrapper { PGN m; int see; int hist; bool is_killer; bool is_pv; };
        std::vector<MoveWrapper> wrapped;
        wrapped.reserve(moves.size());
        for(size_t i=0;i<moves.size();++i){
            MoveWrapper w;
            w.m = moves[i];
            w.see = static_exchange_eval(moves[i], simulate_board);
            uint32_t key = move_key(moves[i]);
            w.hist = history.count(key) ? history.at(key) : 0;
            w.is_killer = false;
            for(const auto &k : killers[ply]){ if(k == moves[i]) { w.is_killer = true; break; } }
            w.is_pv = (have_pv_move && moves[i] == pv_move);
            wrapped.push_back(std::move(w));
        }

        std::sort(wrapped.begin(), wrapped.end(), [](const MoveWrapper &a, const MoveWrapper &b){
            if(a.is_pv != b.is_pv) return a.is_pv; // pv 우선
            if(a.see != b.see) return a.see > b.see; // SEE 우선
            if(a.is_killer != b.is_killer) return a.is_killer; // 킬러 우선
            if(a.hist != b.hist) return a.hist > b.hist; // 히스토리 휴리스틱
            return false;
        });

        // 정렬된 결과를 원래 moves 벡터에 복사
        for(size_t i=0;i<wrapped.size();++i) moves[i] = std::move(wrapped[i].m);
    
        int best = 0;
        bool has_best = false;
        bool maximizing = (player == cT);
        auto base = simulate_board.getPosition();

        // PV 출력용 변수 준비
        PGN best_move;
        std::vector<PGN> best_child_pv;

        if (maximizing) {
            for (auto &mv : moves) {
                std::vector<PGN> child_pv;
                // update hash incrementally, apply move
                update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                simulate_board.updatePiece(mv);
                // 엔진의 승리판정 사용
                victoryType vt = simulate_board.getWhoIsVictory();
                int score;
                if(vt == victoryType::WHITE){
                    score = (cT == colorType::WHITE) ? (MATE_SCORE - ply) : (-MATE_SCORE + ply);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player); // revert
                } else if(vt == victoryType::BLACK){
                    score = (cT == colorType::BLACK) ? (MATE_SCORE - ply) : (-MATE_SCORE + ply);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player); // revert
                } else {
                    // recurse
                    score = minimax_search(depth - 1, (player == colorType::WHITE ? colorType::BLACK : colorType::WHITE), alpha, beta, ply+1, child_pv);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player); // revert
                }
                if (!has_best || score > best) {
                    best = score;
                    best_move = mv;
                    best_child_pv = child_pv;
                    has_best = true;
                }
                if (has_best && best > alpha) alpha = best;
                if (has_best && alpha >= beta) {
                    // record killer & history
                    record_killer(ply, mv);
                    record_history(mv, ply);
                    break;
                }
            }
        } else {
            for (auto &mv : moves) {
                std::vector<PGN> child_pv;
                update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                simulate_board.updatePiece(mv);
                victoryType vt = simulate_board.getWhoIsVictory();
                int score;
                if(vt == victoryType::WHITE){
                    score = (cT == colorType::WHITE) ? (MATE_SCORE - ply) : (-MATE_SCORE + ply);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else if(vt == victoryType::BLACK){
                    score = (cT == colorType::BLACK) ? (MATE_SCORE - ply) : (-MATE_SCORE + ply);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else {
                    score = minimax_search(depth - 1, (player == colorType::WHITE ? colorType::BLACK : colorType::WHITE), alpha, beta, ply+1, child_pv);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                }
                if (!has_best || score < best) {
                    best = score;
                    best_move = mv;
                    best_child_pv = child_pv;
                    has_best = true;
                }
                if (has_best && best < beta) beta = best;
                if (has_best && alpha >= beta) {
                    // record killer & history
                    record_killer(ply, mv);
                    record_history(mv, ply);
                    break;
                }
            }
        }

        if(!has_best){
            pv_out.clear();
            return valueForBot();
        }

        // 트랜스포지션 테이블에 저장
        TTEntry e;
        e.value = best;
        e.depth = depth;
        if(best <= original_alpha) e.flag = 2; // upperbound
        else if(best >= original_beta) e.flag = 1; // lowerbound
        else e.flag = 0; // exact
        e.best = best_move;
        tt_store(current_zobrist, e);

        // pv_out 조립
        pv_out.clear();
        if(best_move.getMoveType() != moveType::NONE){
            pv_out.push_back(best_move);
            pv_out.insert(pv_out.end(), best_child_pv.begin(), best_child_pv.end());
        }
    
        if ((maximizing && best == std::numeric_limits<int>::min()) || (!maximizing && best == std::numeric_limits<int>::max())) {
            // 모든 후보 적용 실패 시 안전 복구(fallback)로 현재 시뮬레이션 보드 기준 값 반환
            return valueForBot();
        }
        return best;
    }

    std::vector<PGN> minimax::generate_captures_and_promotions(colorType player) {
        std::vector<PGN> res;
        // 보드에서 캡처/승격 수집
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = simulate_board.at(f, r);
                if(p.getPieceType() == pieceType::NONE) continue;
                if(p.getColor() != player) continue;
                    auto moves = simulate_board.calcLegalMovesInOnePiece(player, f, r, false); // 이동 & 승격 (합법수만)
                for(const auto &m : moves){
                    // 대상 칸이 상대 기물로 점유되어 캡처가 되는 수 혹은 승격 수를 포함
                    auto to = m.getToSquare();
                    const piece &dest = simulate_board.at(to.first, to.second);
                    if(dest.getPieceType() != pieceType::NONE && dest.getColor() != player) res.push_back(m);
                    else if(m.getMoveType() == moveType::PROMOTE) res.push_back(m);
                }
            }
        }
        return res;
    }

    int minimax::quiescence(int alpha, int beta, int ply_depth, colorType player){
        // 퀴센스는 root_pv를 수정하지 않으며 현재 simulate_board 상태를 사용

        nodes_searched++;
        const int MAX_Q_DEPTH = 32;
        if(ply_depth > MAX_Q_DEPTH) return valueForBot();

        int stand_pat = valueForBot();
        bool maximizing = (player == cT);

        if(maximizing){
            if(stand_pat >= beta) return stand_pat;
            if(alpha < stand_pat) alpha = stand_pat;
        } else {
            if(stand_pat <= alpha) return stand_pat;
            if(beta > stand_pat) beta = stand_pat;
        }

        auto moves = generate_captures_and_promotions(player);
        if(moves.empty()){
            return stand_pat;
        }

        // SEE 기준 — 계산을 미리 해서 정렬 시 중복 호출을 피함
        struct QMoveWrap { PGN m; int see; };
        std::vector<QMoveWrap> qwrap;
        qwrap.reserve(moves.size());
        for(const auto &mv : moves){
            qwrap.push_back({mv, static_exchange_eval(mv, simulate_board)});
        }
        std::sort(qwrap.begin(), qwrap.end(), [](const QMoveWrap &a, const QMoveWrap &b){
            return a.see > b.see;
        });
        // 정렬 결과를 moves로 복원
        moves.clear();
        moves.reserve(qwrap.size());
        for(auto &w : qwrap) moves.push_back(std::move(w.m));

        auto base = simulate_board.getPosition();
        colorType other = (player == colorType::WHITE ? colorType::BLACK : colorType::WHITE);

        if(maximizing){
            for(const auto &mv : moves){
                // update zobrist + apply move
                update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                simulate_board.updatePiece(mv);
                victoryType vt = simulate_board.getWhoIsVictory();
                int score_q;
                if(vt == victoryType::WHITE){
                    score_q = (cT == colorType::WHITE) ? (MATE_SCORE - ply_depth) : (-MATE_SCORE + ply_depth);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else if(vt == victoryType::BLACK){
                    score_q = (cT == colorType::BLACK) ? (MATE_SCORE - ply_depth) : (-MATE_SCORE + ply_depth);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else {
                    score_q = quiescence(alpha, beta, ply_depth+1, other);
                    // undo
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                }

                if(score_q > alpha) alpha = score_q;
                if(alpha >= beta){
                    record_killer(ply_depth, mv);
                    record_history(mv, ply_depth);
                    return alpha;
                }
            }
            return alpha;
        } else {
            for(const auto &mv : moves){
                update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                simulate_board.updatePiece(mv);
                victoryType vt = simulate_board.getWhoIsVictory();
                int score_q;
                if(vt == victoryType::WHITE){
                    score_q = (cT == colorType::WHITE) ? (MATE_SCORE - ply_depth) : (-MATE_SCORE + ply_depth);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else if(vt == victoryType::BLACK){
                    score_q = (cT == colorType::BLACK) ? (MATE_SCORE - ply_depth) : (-MATE_SCORE + ply_depth);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                } else {
                    score_q = quiescence(alpha, beta, ply_depth+1, other);
                    simulate_board.undoBoard();
                    update_zobrist_for_move(current_zobrist, mv, simulate_board, player);
                }

                if(score_q < beta) beta = score_q;
                if(alpha >= beta){
                    record_killer(ply_depth, mv);
                    record_history(mv, ply_depth);
                    return beta;
                }
            }
            return beta;
        }
    }
    PGN minimax::getBestMove(position curr_pos, int depth){
        simulate_board = chessboard(curr_pos);
        offset_board = curr_pos;

        root_pv.clear();

        // follow_turn mode: adapt to position.turn_right; otherwise require match
        if(follow_turn){
            cT = curr_pos.turn_right;
        } else {
            if(curr_pos.turn_right != cT) return PGN();
        }

        // initialize incremental zobrist (includes side-to-move constant for the root player)
        current_zobrist = compute_zobrist(simulate_board.getPosition()) ^ zobrist_side[(cT == colorType::WHITE) ? 0 : 1];

        if(!iterative_deepening){
            // 단발 검색(한 번에 전체 깊이 탐색)
            std::vector<PGN> pv;
            (void)minimax_search(depth, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
            if(!pv.empty()) return pv[0];
            return PGN();
        }

        // iterative deepening with PV-first
        std::vector<PGN> pv;
        PGN bestMove;
        int last_score = 0;
        for(int d = 1; d <= depth; ++d){
            pv.clear();
            int score;
            if(!use_aspiration || d == 1){
                score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
            } else {
                // aspiration window around last_score
                int window = aspiration_window_base;
                int alpha = last_score - window;
                int beta  = last_score + window;
                score = minimax_search(d, cT, alpha, beta, 0, pv);
                if(score <= alpha || score >= beta){
                    // failed aspiration - full re-search
                    score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
                }
            }
            last_score = score;
            if(!pv.empty()) {
                root_pv = pv; // update root pv for next iteration
                bestMove = pv[0];
            }
        }

        return bestMove;
    }

    std::vector<PGN> minimax::getBestLine(position curr_pos, int depth){
        // prepare simulate board and PV storage
        simulate_board = chessboard(curr_pos);
        offset_board = curr_pos;
        root_pv.clear();

        std::vector<PGN> pv;

        // follow_turn mode: adapt to position.turn_right; otherwise require match
        if(follow_turn){
            cT = curr_pos.turn_right;
        } else {
            if(curr_pos.turn_right != cT) return {};
        }

        // initialize zobrist for PV generation
        current_zobrist = compute_zobrist(simulate_board.getPosition()) ^ zobrist_side[(cT == colorType::WHITE) ? 0 : 1];

        if(!iterative_deepening){
            (void)minimax_search(depth, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
        } else {
            int last_score = 0;
            for(int d = 1; d <= depth; ++d){
                pv.clear();
                int score;
                if(!use_aspiration || d == 1){
                    score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
                } else {
                    int window = aspiration_window_base;
                    int alpha = last_score - window;
                    int beta  = last_score + window;
                    score = minimax_search(d, cT, alpha, beta, 0, pv);
                    if(score <= alpha || score >= beta){
                        score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
                    }
                }
                last_score = score;
                if(!pv.empty()) root_pv = pv;
            }
        }

        // PV only (no prefix log)
        return pv;
    }

    calcInfo minimax::getCalcInfo(position curr_pos, int depth)
    {
        calcInfo info{};

        // prepare simulate board and PV storage
        simulate_board = chessboard(curr_pos);
        offset_board = curr_pos;
        root_pv.clear();

        std::vector<PGN> pv;

        // follow_turn mode: adapt to position.turn_right; otherwise require match
        if(follow_turn){
            cT = curr_pos.turn_right;
        } else {
            if(curr_pos.turn_right != cT) return info;
        }

        // initialize zobrist for PV generation
        current_zobrist = compute_zobrist(simulate_board.getPosition()) ^ zobrist_side[(cT == colorType::WHITE) ? 0 : 1];

        int final_score_bot = 0;
        if(!iterative_deepening){
            final_score_bot = minimax_search(depth, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
        } else {
            int last_score = 0;
            for(int d = 1; d <= depth; ++d){
                pv.clear();
                int score;
                if(!use_aspiration || d == 1){
                    score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
                } else {
                    int window = aspiration_window_base;
                    int alpha = last_score - window;
                    int beta  = last_score + window;
                    score = minimax_search(d, cT, alpha, beta, 0, pv);
                    if(score <= alpha || score >= beta){
                        score = minimax_search(d, cT, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, pv);
                    }
                }
                last_score = score;
                if(!pv.empty()) root_pv = pv;
            }
            final_score_bot = last_score;
        }

        // convert to the same convention as eval_pos(): + = white better, - = black better
        info.eval_val = (cT == colorType::WHITE) ? final_score_bot : -final_score_bot;

        if(!pv.empty()) info.bestMove = pv[0];
        else info.bestMove = PGN();

        // PV only (no prefix log)
        info.line = pv;

        return info;
    }
} // namespace agent


