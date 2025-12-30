#!/usr/bin/env python3
"""
Legacy pure-Python bot implementations moved here for reference.
"""
from __future__ import annotations
import random
from typing import Tuple, Optional, List, Dict, Any
from adapter import ChessEngineAdapter


class SimpleBot:
    """합법적인 행동을 무작위로 선택하는 간단한 봇"""
    def __init__(self, engine: ChessEngineAdapter, color: str = "black"):
        self.engine = engine
        self.color = color

    def get_best_move(self) -> bool:
        current_turn = self.engine.turn
        if current_turn != self.color:
            return False

        actions = []
        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                for tf, tr in targets:
                    actions.append(("move", f, r, tf, tr))

        placements = self.engine.legal_placements(self.color)
        for piece_type, f, r in placements:
            actions.append(("drop", piece_type, f, r))

        successions = self.engine.legal_successions(self.color)
        for f, r in successions:
            actions.append(("succession", f, r))

        if not actions:
            return False
        action = random.choice(actions)

        if action[0] == "move":
            _, src_f, src_r, dst_f, dst_r = action
            if self.engine.move((src_f, src_r), (dst_f, dst_r)):
                promotion_opts = self.engine.promotion_options((src_f, src_r), (dst_f, dst_r))
                if promotion_opts:
                    chosen_piece = "Q" if "Q" in promotion_opts else promotion_opts[0]
                    return self.engine.promote_move((src_f, src_r), (dst_f, dst_r), chosen_piece)
                return True
            return False
        elif action[0] == "drop":
            _, piece_type, f, r = action
            return self.engine.drop(piece_type, f, r)
        elif action[0] == "succession":
            _, f, r = action
            return self.engine.succession(f, r)
        return False


class WeightedBot(SimpleBot):
    def __init__(self, engine: ChessEngineAdapter, color: str = "black"):
        super().__init__(engine, color)

    def get_best_move(self) -> bool:
        current_turn = self.engine.turn
        if current_turn != self.color:
            return False

        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                if targets:
                    tf, tr = random.choice(targets)
                    if self.engine.move((f, r), (tf, tr)):
                        promotion_opts = self.engine.promotion_options((f, r), (tf, tr))
                        if promotion_opts:
                            chosen_piece = "Q" if "Q" in promotion_opts else promotion_opts[0]
                            if self.engine.promote_move((f, r), (tf, tr), chosen_piece):
                                return True
                        else:
                            return True

        placements = self.engine.legal_placements(self.color)
        if placements:
            piece_type, f, r = random.choice(placements)
            if self.engine.drop(piece_type, f, r):
                return True

        successions = self.engine.legal_successions(self.color)
        if successions:
            f, r = random.choice(successions)
            if self.engine.succession(f, r):
                return True
        return False


class NegamaxBot:
    PIECE_VALUES = { }
    def __init__(self, engine: ChessEngineAdapter, color: str = "black", depth: int = 3):
        self.engine = engine
        self.color = color
        self.depth = depth
        self.nodes_searched = 0

    # Full implementation omitted for brevity — original code preserved in repository history.
