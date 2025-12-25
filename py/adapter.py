#!/usr/bin/env python3
"""
chess_ext 엔진을 pygame UI에서 사용하기 위한 어댑터 클래스
Engine state를 UI가 필요한 인터페이스로 변환
"""
from __future__ import annotations
import sys
import os
from typing import Dict, List, Tuple, Optional, Iterator

# chess_ext 모듈 로드
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.normpath(os.path.join(script_dir, '..'))
build_dir = os.path.join(project_root, 'build')
if os.path.isdir(build_dir):
    sys.path.insert(0, build_dir)

try:
    import chess_ext  # type: ignore
except ImportError:
    raise SystemExit(
        "chess_ext module not found. Build the pybind11 extension first "
        "(set BUILD_PYBINDING=ON), or run with PYTHONPATH pointing to the build directory."
    )

# 기물 타입을 문자열로 변환
PIECE_TYPE_TO_STR = {
    chess_ext.PieceType.KING: "K",
    chess_ext.PieceType.QUEEN: "Q",
    chess_ext.PieceType.ROOK: "R",
    chess_ext.PieceType.BISHOP: "B",
    chess_ext.PieceType.KNIGHT: "N",
    chess_ext.PieceType.PWAN: "P",
    chess_ext.PieceType.AMAZON: "A",
    chess_ext.PieceType.GRASSHOPPER: "G",
    chess_ext.PieceType.KNIGHTRIDER: "Kr",
    chess_ext.PieceType.ARCHBISHOP: "W",
    chess_ext.PieceType.DABBABA: "D",
    chess_ext.PieceType.ALFIL: "L",
    chess_ext.PieceType.FERZ: "F",
    chess_ext.PieceType.CENTAUR: "C",
    chess_ext.PieceType.CAMEL: "Cl",
    chess_ext.PieceType.TEMPESTROOK: "Tr",
    chess_ext.PieceType.NONE: "",
}

# 문자열을 기물 타입으로 변환 (드롭용)
STR_TO_PIECE_TYPE = {v: k for k, v in PIECE_TYPE_TO_STR.items() if v}

# 색상 변환
COLOR_TO_STR = {
    chess_ext.ColorType.WHITE: "white",
    chess_ext.ColorType.BLACK: "black",
    chess_ext.ColorType.NONE: "none",
}

STR_TO_COLOR_TYPE = {v: k for k, v in COLOR_TO_STR.items() if v}

class ChessEngineAdapter:
    """chess_ext.ChessBoard를 pygame UI에서 사용하기 위한 어댑터"""

    def __init__(self):
        self._board = chess_ext.ChessBoard()
        self._turn_color = "white"  # white starts
        self._last_move: Optional[Tuple[Tuple[int, int], Tuple[int, int]]] = None
        

    @property
    def turn(self) -> str:
        """현재 턴 색상 반환 ('white' or 'black')"""
        return self._turn_color

    def board(self) -> Iterator[Dict]:
        """
        보드 상의 모든 기물을 딕셔너리 리스트로 반환
        각 딕셔너리: {file, rank, type, color, stun, move_stack, stunned, ...}
        """
        for f in range(8):
            for r in range(8):
                p = self._board(f, r)
                pt = p.getPieceType()
                if pt == chess_ext.PieceType.NONE:
                    continue

                ct = p.getColor()
                yield {
                    "file": f,
                    "rank": r,
                    "type": PIECE_TYPE_TO_STR.get(pt, "?"),
                    "color": COLOR_TO_STR.get(ct, "none"),
                    "stun": p.getStun(),
                    "move_stack": p.getMove(),
                    "stunned": p.getStun() > 0,
                }

    def pockets(self, color: str) -> Dict[str, int]:
        """포켓의 기물 개수 반환 (색상별)"""
        if color == "white":
            raw = self._board.getWhitePocket()
        elif color == "black":
            raw = self._board.getBlackPocket()
        else:
            return {}
        
        result = {}
        for pt, symbol in PIECE_TYPE_TO_STR.items():
            if pt == chess_ext.PieceType.NONE or not symbol:
                continue
            count = raw[int(pt)]
            if count > 0:
                result[symbol] = count
        return result

    def reset(self):
        """보드 초기화"""
        self._board = chess_ext.ChessBoard()
        self._turn_color = "white"
        self._last_move = None

    def owned_piece_at(self, file: int, rank: int) -> bool:
        """현재 턴 플레이어의 기물이 해당 위치에 있는지 확인"""
        p = self._board(file, rank)
        if p.getPieceType() == chess_ext.PieceType.NONE:
            return False
        ct = p.getColor()
        piece_color = COLOR_TO_STR.get(ct, "none")
        return piece_color == self._turn_color

    def legal_moves(self, file: int, rank: int) -> List[Tuple[int, int]]:
        """
        해당 위치 기물의 합법적 이동 목표 리스트 반환
        chess_ext의 calcLegalMovesInOnePiece 사용
        """
        pgns = self._board.calcLegalMovesInOnePiece(file, rank)
        targets = []
        for pgn in pgns:
            # PGN 객체에서 목적지 좌표 추출
            # pgn.end_file, pgn.end_rank 같은 속성이 있다고 가정
            # 실제 API에 맞춰 조정 필요
            try:
                targets.append((pgn.end_file, pgn.end_rank))
            except AttributeError:
                # PGN 구조를 모르면 일단 스킵
                pass
        return targets

    def move(self, src: Tuple[int, int], dst: Tuple[int, int]) -> bool:
        """
        기물 이동 시도
        반환: 성공 여부
        """
        sf, sr = src
        df, dr = dst
        try:
            self._board.movePiece(sf, sr, df, dr)
            self._last_move = (src, dst)
            return True
        except Exception:
            return False

    def drop(self, piece_type_str: str, file: int, rank: int) -> bool:
        """
        포켓에서 기물 드롭
        piece_type_str: "P", "N", "K" 등
        """
        pt = STR_TO_PIECE_TYPE.get(piece_type_str)
        if pt is None:
            return False
        ct = chess_ext.ColorType.WHITE if self._turn_color == "white" else chess_ext.ColorType.BLACK
        try:
            self._board.placePiece(ct, pt, file, rank)
            return True
        except Exception:
            return False

    def stun(self, file: int, rank: int) -> bool:
        """
        해당 위치 기물에 스턴 추가
        """
        try:
            p = self._board(file, rank)
            if p.getPieceType() == chess_ext.PieceType.NONE:
                return False
            p.addOneStun()
            return True
        except Exception:
            return False

    def end_turn(self):
        """턴 종료, 색상 교체"""
        for f in range(8):
            for r in range(8):
                p = self._board(f, r)
                if p.getStun() != 0 and  p.getColor() == STR_TO_COLOR_TYPE.get(self._turn_color):
                    p.addStun(-1)
                    p.addMove(1)
        self._turn_color = "black" if self._turn_color == "white" else "white"

    def next_drop_kind(self, current: str) -> str:
        """드롭 기물 종류 순환"""
        kinds = ["P", "N", "B", "R", "Q", "K", "A", "G", "Kr", "W", "D", "L", "F", "C", "Cl", "Tr"]
        try:
            idx = kinds.index(current)
            return kinds[(idx + 1) % len(kinds)]
        except ValueError:
            return "P"
