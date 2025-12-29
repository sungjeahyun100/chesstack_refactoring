#include "piece_spec.hpp"

namespace specs {

// Direction constants
static const std::vector<std::pair<int, int>> KNIGHT_DIRECTIONS = {
    {1, 2}, {2, 1}, {2, -1}, {1, -2},
    {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
};

static const std::vector<std::pair<int, int>> BISHOP_DIRECTIONS = {
    {1, 1}, {1, -1}, {-1, -1}, {-1, 1}
};

static const std::vector<std::pair<int, int>> ROOK_DIRECTIONS = {
    {0, 1}, {1, 0}, {0, -1}, {-1, 0}
};

static const std::vector<std::pair<int, int>> EIGHT_WAY_DIRECTIONS = {
    {0, 1}, {1, 1}, {1, 0}, {1, -1},
    {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
};

static const std::vector<std::pair<int, int>> DABBABA_DIRECTIONS = {
    {0, 2}, {2, 0}, {0, -2}, {-2, 0}
};

static const std::vector<std::pair<int, int>> ALFIL_DIRECTIONS = {
    {2, 2}, {2, -2}, {-2, -2}, {-2, 2}
};

static const std::vector<std::pair<int, int>> CAMEL_DIRECTIONS = {
    {1, 3}, {3, 1}, {3, -1}, {1, -3},
    {-1, -3}, {-3, -1}, {-3, 1}, {-1, 3}
};

static inline int colorIndex(colorType ct) {
    switch (ct) {
        case colorType::WHITE: return 0;
        case colorType::BLACK: return 1;
        default: return 2; // NONE or others map to a neutral bucket
    }
}

static PieceSpec makeSpec(pieceType pt, colorType ct) {
    PieceSpec spec;
    switch(pt) {
        case pieceType::KING:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, EIGHT_WAY_DIRECTIONS, 1));
            break;
        case pieceType::QUEEN:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, EIGHT_WAY_DIRECTIONS));
            break;
        case pieceType::BISHOP:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, BISHOP_DIRECTIONS));
            break;
        case pieceType::KNIGHT:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, KNIGHT_DIRECTIONS, 1));
            break;
        case pieceType::ROOK:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, ROOK_DIRECTIONS));
            break;
        case pieceType::PWAN: {
            if (ct == colorType::WHITE) {
                spec.moves.push_back(moveChunk(threatType::MOVE, {0,0}, {{0,1}}, 1));
                spec.moves.push_back(moveChunk(threatType::TAKE, {0,0}, {{-1,1},{1,1}}, 1));
            } else {
                spec.moves.push_back(moveChunk(threatType::MOVE, {0,0}, {{0,-1}}, 1));
                spec.moves.push_back(moveChunk(threatType::TAKE, {0,0}, {{-1,-1},{1,-1}}, 1));
            }
            spec.isPromotable = true;
            spec.promotePool = {pieceType::QUEEN, pieceType::ROOK, pieceType::BISHOP, pieceType::KNIGHT, pieceType::AMAZON};
            int targetRank = (ct == colorType::WHITE) ? 7 : 0;
            for (int file = 0; file < BOARDSIZE; ++file) spec.promotableSquares.push_back({file, targetRank});
            break;
        }
        case pieceType::AMAZON:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, KNIGHT_DIRECTIONS, 1));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, EIGHT_WAY_DIRECTIONS));
            break;
        case pieceType::GRASSHOPPER:
            spec.moves.push_back(moveChunk(threatType::TAKEJUMP, {0,0}, EIGHT_WAY_DIRECTIONS));
            break;
        case pieceType::KNIGHTRIDER:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, KNIGHT_DIRECTIONS));
            break;
        case pieceType::ARCHBISHOP:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, KNIGHT_DIRECTIONS, 1));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, BISHOP_DIRECTIONS));
            break;
        case pieceType::DABBABA:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, DABBABA_DIRECTIONS, 1));
            break;
        case pieceType::ALFIL:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, ALFIL_DIRECTIONS, 1));
            break;
        case pieceType::FERZ:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, BISHOP_DIRECTIONS, 1));
            break;
        case pieceType::CENTAUR:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, EIGHT_WAY_DIRECTIONS, 1));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, KNIGHT_DIRECTIONS, 1));
            break;
        case pieceType::CAMEL:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {0,0}, CAMEL_DIRECTIONS, 1));
            break;
        case pieceType::TEMPESTROOK:
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {1,1}, {{0,1},{1,0}}));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {1,-1}, {{1,0},{0,-1}}));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {-1,1}, {{0,1},{-1,0}}));
            spec.moves.push_back(moveChunk(threatType::TAKEMOVE, {-1,-1}, {{0,-1},{-1,0}}));
            break;
        default:
            break;
    }
    return spec;
}

const PieceSpec& get(pieceType pt, colorType ct) {
    static std::array<std::array<bool, NUMBER_OF_PIECEKIND>, 3> built{};
    static std::array<std::array<PieceSpec, NUMBER_OF_PIECEKIND>, 3> cache;
    const int ci = colorIndex(ct);
    const int pi = static_cast<int>(pt);
    if (!built[ci][pi]) {
        cache[ci][pi] = makeSpec(pt, ct);
        built[ci][pi] = true;
    }
    return cache[ci][pi];
}

// Convenience accessors
const std::vector<moveChunk>& moves(pieceType pt, colorType ct) {
    return get(pt, ct).moves;
}

bool isPromotable(pieceType pt) {
    return get(pt, colorType::WHITE).isPromotable;
}

const std::vector<pieceType>& promotePool(pieceType pt) {
    return get(pt, colorType::WHITE).promotePool;
}

const std::vector<std::pair<int,int>>& promotableSquares(pieceType pt, colorType ct) {
    return get(pt, ct).promotableSquares;
}

} // namespace specs
