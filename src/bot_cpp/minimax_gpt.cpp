#include "agent.hpp"
#include <cmath>

namespace agent {

// piece values (same scale as existing engine; duplicated here for isolation)
static int g_piece_value(pieceType pt) {
    switch(pt){
        case pieceType::KING: return 400;
        case pieceType::QUEEN: return 900;
        case pieceType::ROOK: return 500;
        case pieceType::BISHOP: return 330;
        case pieceType::KNIGHT: return 320;
        case pieceType::PWAN: return 100;
        case pieceType::AMAZON: return 1400;
        case pieceType::GRASSHOPPER: return 280;
        case pieceType::KNIGHTRIDER: return 650;
        case pieceType::ARCHBISHOP: return 800;
        case pieceType::DABBABA: return 250;
        case pieceType::ALFIL: return 250;
        case pieceType::FERZ: return 150;
        case pieceType::CENTAUR: return 700;
        case pieceType::CAMEL: return 450;
        case pieceType::TEMPESTROOK: return 700;
        default: return 0;
    }
}

// Smooth placement score helper: exponential decay by distance
static double placement_decay(double base_value, int f, int r){
    const double lambda = 0.35; // decay rate; tunable
    double dx = 4.5 - static_cast<double>(f);
    double dy = 4.5 - static_cast<double>(r);
    double dist = std::sqrt(dx*dx + dy*dy);
    return base_value * std::exp(-lambda * dist);
}

// Internal minimax subclass that overrides eval_pos/placement_score
struct minimax_gpt_impl : public minimax {
    minimax_gpt_impl(colorType ct) : minimax(ct) {}
    minimax_gpt_impl() : minimax() {}
    virtual int eval_pos(const position& pos) const override {
        // Weighted sum of components (centipawn-ish scale)
        const double w_M = 1.0;        // 기물 자체 가치
        const double w_Mob = 15.0;     // 이동성
        const double w_Res = 40.0;     // 스택 가치 (move/stun stacks)
        const double w_Place = 30.0;   // 착수 위치의 중앙 거리 대비 가중치
        const double w_Thr = 50.0;     // threat/capture potential 기물 공격 가중치
        const double w_Turn = 5.0;    // side-to-move bonus 템포 가중치
        const double w_Royal = 8000.0; // last-royal bonus 마지막 남은 로얄 피스 가중치

        chessboard tmp(pos);
        double Mat = 0.0;
        double Mob = 0.0;
        double Res = 0.0;
        double Place = 0.0;
        double Thr = 0.0;

        // material & resource & basic counts
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = pos.board[f][r];
                if(p.getPieceType() == pieceType::NONE) continue;
                int val = g_piece_value(p.getPieceType());
                if(p.getColor() == colorType::WHITE) Mat += val;
                else Mat -= val;

                int mvstk = p.getMove();
                int stnstk = p.getStun();
                double res_contrib = static_cast<double>(mvstk) - 0.5 * static_cast<double>(stnstk);
                if(p.getColor() == colorType::WHITE) Res += res_contrib; else Res -= res_contrib;
            }
        }

        // pockets: add pocket material contribution (approx)
        for(int i=0;i<NUMBER_OF_PIECEKIND;++i){
            int wcnt = pos.whitePocket[i];
            int bcnt = pos.blackPocket[i];
            int pv = g_piece_value(static_cast<pieceType>(i));
            Mat += pv * wcnt;
            Mat -= pv * bcnt;
        }

        // Mobility and threat: approximate by sampling legal moves per piece
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = tmp.at(f,r);
                if(p.getPieceType() == pieceType::NONE) continue;
                auto moves = tmp.calcLegalMovesInOnePiece(p.getColor(), f, r, true);
                int mvcount = static_cast<int>(moves.size());
                int capped = std::min(mvcount, 32);
                double mob_contrib = static_cast<double>(capped);
                if(p.getColor() == colorType::WHITE) Mob += mob_contrib; else Mob -= mob_contrib;

                for(const auto &m : moves){
                    if(m.getMoveType() == moveType::PROMOTE) continue;
                    auto to = m.getToSquare();
                    const piece &vict = tmp.at(to.first, to.second);
                    if(!vict.isEmpty() && vict.getColor() != p.getColor()){
                        int v = g_piece_value(vict.getPieceType());
                        if(p.getColor() == colorType::WHITE) Thr += v; else Thr -= v;
                    }
                }
            }
        }

        // Placement bias: small bonus for central placements currently on board
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                const piece &p = pos.board[f][r];
                if(p.getPieceType() == pieceType::NONE) continue;
                double base = static_cast<double>(g_piece_value(p.getPieceType()));
                double dx = 4.5 - static_cast<double>(f);
                double dy = 4.5 - static_cast<double>(r);
                double dist = std::sqrt(dx*dx + dy*dy);
                const double lambda = 0.35;
                double dec = base * std::exp(-lambda * dist);
                if(p.getColor() == colorType::WHITE) Place += dec; else Place -= dec;
            }
        }

        // Royal last-piece bonus
        int white_royal = 0, black_royal = 0;
        for(int f=0; f<BOARDSIZE; ++f) for(int r=0; r<BOARDSIZE; ++r){
            const piece &p = pos.board[f][r];
            if(p.getPieceType() == pieceType::NONE) continue;
            if(p.getIsRoyal()){
                if(p.getColor() == colorType::WHITE) ++white_royal; else ++black_royal;
            }
        }
        double Royal = 0.0;
        if(white_royal == 1 && black_royal == 0) Royal = w_Royal;
        if(black_royal == 1 && white_royal == 0) Royal = -w_Royal;

        // infer side-to-move from explicit flag `turn_right` in position
        double Turn = (pos.turn_right == colorType::WHITE) ? 1.0 : -1.0;

        double eval = 0.0;
        eval += w_M * Mat;
        eval += w_Mob * Mob;
        eval += w_Res * Res;
        eval += w_Place * Place;
        eval += w_Thr * Thr;
        eval += w_Turn * Turn;
        eval += Royal;

        return static_cast<int>(std::round(eval));
    }

    double placement_score(const PGN &pgn, colorType player) const {
        auto from = pgn.getFromSquare();
        int f = from.first, r = from.second;
        int pv = g_piece_value(pgn.getPieceType());
        double base = static_cast<double>(pv);
        double dec = placement_decay(base, f, r);
        return (player == colorType::WHITE) ? dec : -dec;
    }
};

// Public wrapper that fulfills the `bot` interface and delegates to internal impl
struct minimax_GPTproposed::Impl {
    std::unique_ptr<minimax_gpt_impl> mptr;
    Impl(colorType ct) { mptr = std::make_unique<minimax_gpt_impl>(ct); }
    Impl() { mptr = std::make_unique<minimax_gpt_impl>(); }
};
minimax_GPTproposed::minimax_GPTproposed(colorType ct) : impl(std::make_unique<Impl>(ct)){}
minimax_GPTproposed::minimax_GPTproposed() : impl(std::make_unique<Impl>()){}
minimax_GPTproposed::~minimax_GPTproposed() = default;

int minimax_GPTproposed::eval_pos(const position& pos) const {
    return impl->mptr->eval_pos(pos);
}

PGN minimax_GPTproposed::getBestMove(position curr_pos, int depth) {
    return impl->mptr->getBestMove(curr_pos, depth);
}

std::vector<PGN> minimax_GPTproposed::getBestLine(position curr_pos, int depth)
{
    return impl->mptr->getBestLine(curr_pos, depth);
}

// Forwarding control/inspection helpers
void minimax_GPTproposed::setPlacementSample(size_t k){ impl->mptr->setPlacementSample(k); }
void minimax_GPTproposed::reset_search_data(){ impl->mptr->reset_search_data(); }
void minimax_GPTproposed::setIterativeDeepening(bool v){ impl->mptr->iterative_deepening = v; }
void minimax_GPTproposed::setUseAspiration(bool v){ impl->mptr->use_aspiration = v; }
void minimax_GPTproposed::setAspirationWindowBase(int val){ impl->mptr->aspiration_window_base = val; }
void minimax_GPTproposed::setNodesSearched(uint64_t val){ impl->mptr->nodes_searched = val;}
uint64_t minimax_GPTproposed::getNodesSearched() const { return impl->mptr->nodes_searched; }

} // namespace agent
