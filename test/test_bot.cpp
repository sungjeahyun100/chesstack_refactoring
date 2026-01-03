#include <chess.hpp>
#include <agent.hpp>

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>

static const char* pieceTypeToStr(pieceType pt){
    switch(pt){
        case pieceType::KING: return "K";
        case pieceType::QUEEN: return "Q";
        case pieceType::ROOK: return "R";
        case pieceType::BISHOP: return "B";
        case pieceType::KNIGHT: return "N";
        case pieceType::PWAN: return "P";
        case pieceType::AMAZON: return "A";
        case pieceType::GRASSHOPPER: return "G";
        case pieceType::KNIGHTRIDER: return "Kr";
        case pieceType::ARCHBISHOP: return "W";
        case pieceType::DABBABA: return "D";
        case pieceType::ALFIL: return "L";
        case pieceType::FERZ: return "F";
        case pieceType::CENTAUR: return "C";
        case pieceType::CAMEL: return "Cl";
        case pieceType::TEMPESTROOK: return "Tr";
        default: return "?";
    }
}

int main(){
    // 간단한 테스트 포지션 구성
    chessboard cb;
    cb.setVarientPiece();

    position pos = cb.getPosition();

    // --- Benchmark runner across multiple sample positions ---
    struct Result { int pos_id; int depth; uint64_t nodes; double total_ms; double search_ms; std::string mode; std::string move_str; };

    std::vector<Result> results;

    // helper to build positions from piece lists
    auto make_position = [&](const std::vector<std::tuple<colorType,pieceType,int,int>>& pieces, bool has_varient){
        chessboard cb2;
        if(has_varient) cb2.setVarientPiece();

        // place kings by default if none specified
        bool has_white_king=false, has_black_king=false;
        for(auto &t : pieces){
            colorType c; pieceType p; int f,r; std::tie(c,p,f,r) = t;
            cb2.placePiece(c, p, f, r);
            if(p==pieceType::KING && c==colorType::WHITE) has_white_king=true;
            if(p==pieceType::KING && c==colorType::BLACK) has_black_king=true;
        }
        if(!has_white_king) cb2.placePiece(colorType::WHITE, pieceType::KING, 4, 0);
        if(!has_black_king) cb2.placePiece(colorType::BLACK, pieceType::KING, 4, 7);
        return cb2.getPosition();
    };

    // Sample positions
    std::vector<position> samples;

    // single sample: small middlegame (original)
    samples.push_back(pos);
    // additional samples constructed via helper
    samples.push_back(make_position({
        {colorType::WHITE, pieceType::ROOK, 0,0},
        {colorType::WHITE, pieceType::PWAN, 1,1},
        {colorType::WHITE, pieceType::PWAN, 2,1},
        {colorType::BLACK, pieceType::ROOK, 7,7},
        {colorType::BLACK, pieceType::PWAN, 6,6}
    }, false)); // rook endgame-like

    samples.push_back(make_position({
        {colorType::WHITE, pieceType::QUEEN, 3,3},
        {colorType::WHITE, pieceType::BISHOP, 2,2},
        {colorType::WHITE, pieceType::KNIGHT, 1,2},
        {colorType::BLACK, pieceType::QUEEN, 4,4},
        {colorType::BLACK, pieceType::ROOK, 6,6},
        {colorType::BLACK, pieceType::KNIGHT, 5,5}
    }, false)); // tactical middlegame

    samples.push_back(make_position({
        {colorType::WHITE, pieceType::ROOK, 0,1},
        {colorType::WHITE, pieceType::ROOK, 1,1},
        {colorType::WHITE, pieceType::KNIGHT, 2,2},
        {colorType::BLACK, pieceType::ROOK, 7,6},
        {colorType::BLACK, pieceType::BISHOP, 5,5},
        {colorType::BLACK, pieceType::PWAN, 4,4}
    }, false)); // cramped tactics

    // --- Variant-piece samples (added for testing variant pieces like Amazon, Grasshopper, etc.)
    samples.push_back(make_position({
        {colorType::WHITE, pieceType::AMAZON, 2,2},
        {colorType::WHITE, pieceType::GRASSHOPPER, 3,2},
        {colorType::WHITE, pieceType::KNIGHTRIDER, 1,3},
        {colorType::BLACK, pieceType::ARCHBISHOP, 5,5},
        {colorType::BLACK, pieceType::DABBABA, 6,6}
    }, true)); // variant tactical mix
    samples.push_back(make_position({
        {colorType::WHITE, pieceType::CENTAUR, 1,1},
        {colorType::WHITE, pieceType::CAMEL, 2,1},
        {colorType::WHITE, pieceType::TEMPESTROOK, 3,1},
        {colorType::BLACK, pieceType::ALFIL, 6,6},
        {colorType::BLACK, pieceType::FERZ, 5,6}
    }, true)); // variant endgame-like

    const int maxDepth = 10; // keep reasonable for automated runs
    for(size_t pid=0; pid<samples.size(); ++pid){
        const position &pcur = samples[pid];
        position mutable_pos = pcur; // chessboard ctor expects non-const reference
        chessboard cbx(mutable_pos);
        std::cout << "\n=== Sample Position " << pid+1 << " ===" << std::endl;
        cbx.displayBoard();
        cbx.displayPockets();
        

        // Ensure pieces on this sample have nonzero move stacks for deeper search
        for(int f=0; f<BOARDSIZE; ++f){
            for(int r=0; r<BOARDSIZE; ++r){
                if(!cbx(f,r).isEmpty() && cbx(f,r).getMove() == 0){
                    cbx(f,r).setMove(3);
                }
            }
        }
        // reflect back to position used by bot
        mutable_pos = cbx.getPosition();

        agent::minimax_GPTproposed botx(colorType::WHITE);
        botx.setPlacementSample(30);

        for(int depth = 1; depth <= maxDepth; ++depth){
            auto t0 = std::chrono::high_resolution_clock::now();
            // baseline run (no iterative deepening)
            botx.reset_search_data();
            botx.setIterativeDeepening(false);
            botx.setNodesSearched(0);
            auto tsearch_start = std::chrono::high_resolution_clock::now();
            PGN mv = botx.getBestMove(pcur, depth);
            auto tsearch_end = std::chrono::high_resolution_clock::now();
            auto t1 = std::chrono::high_resolution_clock::now();

            double total_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
            double search_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tsearch_end - tsearch_start).count();

            std::string move_str;
            if(mv.getMoveType() == moveType::NONE) move_str = "NO_MOVE";
            else if(mv.getMoveType() == moveType::ADD){ auto f=mv.getFromSquare(); move_str = std::string("ADD ") + pieceTypeToStr(mv.getPieceType()) + " at(" + std::to_string(f.first)+","+std::to_string(f.second)+")"; }
            else if(mv.getMoveType() == moveType::SUCCESION){auto f=mv.getFromSquare(); move_str = std::string("SUCESSION ") + "at(" + std::to_string(f.first)+","+std::to_string(f.second)+")";}
            else { auto f=mv.getFromSquare(); auto t=mv.getToSquare(); move_str = "from("+std::to_string(f.first)+","+std::to_string(f.second)+")->("+std::to_string(t.first)+","+std::to_string(t.second)+")"; }

            results.push_back({(int)pid+1, depth, botx.getNodesSearched(), total_ms, search_ms, "base", move_str});

            std::cout << "(base) Depth="<<depth<<" nodes="<<botx.getNodesSearched()<<" search="<<std::fixed<<std::setprecision(3)<<search_ms<<"ms total="<<total_ms<<"ms move="<<move_str<<std::endl;

            // PV-first iterative deepening run
            botx.reset_search_data();
            botx.setIterativeDeepening(true);
            botx.setUseAspiration(false);
            botx.setNodesSearched(0);
            tsearch_start = std::chrono::high_resolution_clock::now();
            PGN mv2 = botx.getBestMove(pcur, depth);
            tsearch_end = std::chrono::high_resolution_clock::now();
            t1 = std::chrono::high_resolution_clock::now();

            double total_ms2 = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
            double search_ms2 = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tsearch_end - tsearch_start).count();

            std::string move_str2;
            if(mv2.getMoveType() == moveType::NONE) move_str2 = "NO_MOVE";
            else if(mv2.getMoveType() == moveType::ADD){ auto f=mv2.getFromSquare(); move_str2 = std::string("ADD ") + pieceTypeToStr(mv2.getPieceType()) + " at(" + std::to_string(f.first)+","+std::to_string(f.second)+")"; }
            else if(mv.getMoveType() == moveType::SUCCESION){auto f=mv.getFromSquare(); move_str2 = std::string("SUCESSION ") + "at(" + std::to_string(f.first)+","+std::to_string(f.second)+")";}
            else { auto f=mv2.getFromSquare(); auto t=mv2.getToSquare(); move_str2 = "from("+std::to_string(f.first)+","+std::to_string(f.second)+")->("+std::to_string(t.first)+","+std::to_string(t.second)+")"; }

            results.push_back({(int)pid+1, depth, botx.getNodesSearched(), total_ms2, search_ms2, "pv", move_str2});

            std::cout << "(pv)   Depth="<<depth<<" nodes="<<botx.getNodesSearched()<<" search="<<std::fixed<<std::setprecision(3)<<search_ms2<<"ms total="<<total_ms2<<"ms move="<<move_str2<<std::endl;

            // PV + aspiration
            botx.reset_search_data();
            botx.setIterativeDeepening(true);
            botx.setUseAspiration(false);
            botx.setAspirationWindowBase(50); //centipwans
            botx.setNodesSearched(0);
            tsearch_start = std::chrono::high_resolution_clock::now();
            PGN mv3 = botx.getBestMove(pcur, depth);
            tsearch_end = std::chrono::high_resolution_clock::now();
            t1 = std::chrono::high_resolution_clock::now();

            double total_ms3 = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t1 - t0).count();
            double search_ms3 = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tsearch_end - tsearch_start).count();

            std::string move_str3;
            if(mv3.getMoveType() == moveType::NONE) move_str3 = "NO_MOVE";
            else if(mv3.getMoveType() == moveType::ADD){ auto f=mv3.getFromSquare(); move_str3 = std::string("ADD ") + pieceTypeToStr(mv3.getPieceType()) + " at(" + std::to_string(f.first)+","+std::to_string(f.second)+")"; }
            else if(mv.getMoveType() == moveType::SUCCESION){auto f=mv.getFromSquare(); move_str3 = std::string("SUCESSION ") + "at(" + std::to_string(f.first)+","+std::to_string(f.second)+")";}
            else { auto f=mv3.getFromSquare(); auto t=mv3.getToSquare(); move_str3 = "from("+std::to_string(f.first)+","+std::to_string(f.second)+")->("+std::to_string(t.first)+","+std::to_string(t.second)+")"; }

            results.push_back({(int)pid+1, depth, botx.getNodesSearched(), total_ms3, search_ms3, "pv+asp", move_str3});

            std::cout << "(asp)  Depth="<<depth<<" nodes="<<botx.getNodesSearched()<<" search="<<std::fixed<<std::setprecision(3)<<search_ms3<<"ms total="<<total_ms3<<"ms move="<<move_str3<<std::endl;        }
    }

    // write CSV
    std::ofstream fout("bench_results.csv");
    fout << "pos,depth,mode,nodes,search_ms,total_ms,move\n";
    for(auto &r : results){
        fout << r.pos_id << "," << r.depth << "," << r.mode << "," << r.nodes << "," << r.search_ms << "," << r.total_ms << "," << r.move_str << "\n";
    }
    fout.close();

    std::cout << "\nBench results written to bench_results.csv" << std::endl;

    return 0;
}
