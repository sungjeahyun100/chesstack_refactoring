#!/usr/bin/env python3
"""
Binding-backed bot wrappers.

Keep these thin: they call into the C++ `chess_ext` bots and execute
returned `PGN`s through the existing `ChessEngineAdapter`.
"""
from __future__ import annotations
from typing import Tuple
import chess_ext  # type: ignore
from adapter import ChessEngineAdapter, PIECE_TYPE_TO_STR


class MinimaxBot:
    """Wrapper around `chess_ext.Minimax`.

    Args:
        engine: ChessEngineAdapter instance
        color: 'white' or 'black'
        depth: search depth passed to the C++ bot
    """
    def __init__(self, engine: ChessEngineAdapter, color: str = "black", depth: int = 3):
        self.engine = engine
        self.color = color
        self.depth = depth
        ct = chess_ext.ColorType.WHITE if color == "white" else chess_ext.ColorType.BLACK
        self._bot = chess_ext.Minimax(ct)
        try:
            self._bot.setFollowTurn(True)
        except Exception:
            pass

    def get_best_move(self) -> bool:
        if self.engine.turn != self.color:
            return False

        pgn = self._bot.getBestMove(self.engine._board, int(self.depth))
        try:
            mt = pgn.getMoveType()
        except Exception:
            return False

        if mt == chess_ext.MoveType.NONE:
            return False

        if mt == chess_ext.MoveType.MOVE or mt == chess_ext.MoveType.PROMOTE:
            sf, sr = pgn.getFromSquare()
            df, dr = pgn.getToSquare()
            if mt == chess_ext.MoveType.PROMOTE:
                if not self.engine.move((sf, sr), (df, dr)):
                    return False
                sym = PIECE_TYPE_TO_STR.get(pgn.getPieceType(), "")
                if not sym:
                    return False
                return self.engine.promote_move((sf, sr), (df, dr), sym)
            else:
                return self.engine.move((sf, sr), (df, dr))

        if mt == chess_ext.MoveType.ADD:
            pt = pgn.getPieceType()
            sym = PIECE_TYPE_TO_STR.get(pt, "")
            if not sym:
                return False
            f, r = pgn.getFromSquare()
            return self.engine.drop(sym, f, r)

        if mt == chess_ext.MoveType.SUCCESION:
            f, r = pgn.getFromSquare()
            return self.engine.succession(f, r)

        return False

    def get_best_line(self, depth: int = 0):
        """Return principal variation (list of PGN) from the C++ bot for given depth.

        If `depth` is None, uses the bot's configured depth.
        """
        d = int(self.depth if depth is None else depth)
        try:
            line = self._bot.getBestLine(self.engine._board, d)
            return list(line)
        except Exception:
            return []


class MinimaxGPTBot(MinimaxBot):
    """Wrapper that uses the GPT-proposed minimax implementation."""
    def __init__(self, engine: ChessEngineAdapter, color: str = "black", depth: int = 3):
        self.engine = engine
        self.color = color
        self.depth = depth
        ct = chess_ext.ColorType.WHITE if color == "white" else chess_ext.ColorType.BLACK
        self._bot = chess_ext.MinimaxGPT(ct)
        try:
            self._bot.setFollowTurn(True)
        except Exception:
            pass

    def get_baseline(self) -> int:
        try:
            raw = self._bot.getBaseline(self.engine._board)
        except Exception:
            return 0
        return int(raw if self.color == 'white' else -raw)
    
    def get_best_line(self, depth: int = 0):
        d = int(self.depth if depth == 0 else depth)
        try:
            line = self._bot.getBestLine(self.engine._board, d)
            return list(line)
        except Exception:
            return []


