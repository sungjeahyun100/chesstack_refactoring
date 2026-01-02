from __future__ import annotations
from typing import List

import chess_ext  # type: ignore

from adapter import PIECE_TYPE_TO_STR
from bot import MinimaxBot, MinimaxGPTBot
from ui.state import UIState

# 봇 관련 유틸 모듈: PGN 문자열화, 분석 정보 업데이트, 봇 생성/설정 적용을 책임진다.


def idx_to_alg(file: int, rank: int) -> str:
    return f"{chr(ord('a') + file)}{rank + 1}"


def pgn_to_str(pgn) -> str:
    try:
        mt = pgn.getMoveType()
    except Exception:
        return "?"

    if mt == chess_ext.MoveType.MOVE or mt == chess_ext.MoveType.PROMOTE:
        sf, sr = pgn.getFromSquare()
        df, dr = pgn.getToSquare()
        return f"{idx_to_alg(sf, sr)}->{idx_to_alg(df, dr)}"
    if mt == chess_ext.MoveType.ADD:
        pt = pgn.getPieceType()
        sym = PIECE_TYPE_TO_STR.get(pt, "?")
        f, r = pgn.getFromSquare()
        return f"ADD {sym}@{idx_to_alg(f, r)}"
    if mt == chess_ext.MoveType.SUCCESION:
        f, r = pgn.getFromSquare()
        return f"SUC@{idx_to_alg(f, r)}"
    return "?"


def wrap_moves(moves: List[str], max_chars: int = 40) -> List[str]:
    lines: List[str] = []
    cur = ""
    for m in moves:
        if not cur:
            cur = m
            continue
        if len(cur) + 1 + len(m) <= max_chars:
            cur += " " + m
        else:
            lines.append(cur)
            cur = m
    if cur:
        lines.append(cur)
    return lines


def update_analysis(ui: UIState, bot) -> None:
    """봇의 계산 정보로 UI 분석 패널을 채우고 캐시 플래그를 정리."""
    try:
        info = bot.get_calc_info(ui.depth)
    except Exception:
        info = None
    if info is None:
        ui.analysis_eval = 0
        ui.analysis_best = ""
        ui.analysis_pv = []
        ui.analysis_dirty = False
        return
    try:
        ui.analysis_eval = int(info.eval_val)
    except Exception:
        ui.analysis_eval = 0

    try:
        ui.analysis_best = pgn_to_str(info.bestMove)
    except Exception:
        ui.analysis_best = ""

    try:
        pv = list(info.line)
    except Exception:
        pv = []

    mv_strs = [pgn_to_str(m) for m in pv[:10]]
    ui.analysis_pv = wrap_moves(mv_strs, max_chars=40)
    ui.analysis_dirty = False


def create_bot(engine, ui: UIState):
    """UI 설정을 반영해 봇 인스턴스를 만들고 placement_sample을 적용."""
    if ui.bot_type == "GPTproposed":
        b = MinimaxGPTBot(engine, ui.bot_color, depth=ui.depth)
    else:
        b = MinimaxBot(engine, ui.bot_color, depth=ui.depth)
    try:
        b._bot.setPlacementSample(int(ui.placement_sample))
    except Exception:
        pass
    return b
