#pragma once
#include <vector>
#include <utility>
#include <array>
#include <optional>
#include "chess.hpp"

// Non-destructive flyweight: read-only spec per (pieceType, colorType).
// Built lazily from existing piece initialization logic, then cached.
namespace specs {

struct PieceSpec {
    // NOTE: isRoyal is NOT stored here (dynamic, per-instance per rules).
    bool isPromotable = false;
    std::vector<pieceType> promotePool;
    std::vector<std::pair<int,int>> promotableSquares;
    std::vector<moveChunk> moves;
};

// Returns a const reference to cached spec for given type/color.
// For color-less queries (e.g., isRoyal), pass either color; value is the same.
const PieceSpec& get(pieceType pt, colorType ct);

// Convenience accessors (defined in piece_spec.cpp)
const std::vector<moveChunk>& moves(pieceType pt, colorType ct);
bool isPromotable(pieceType pt);
const std::vector<pieceType>& promotePool(pieceType pt);
const std::vector<std::pair<int,int>>& promotableSquares(pieceType pt, colorType ct);

} // namespace specs
