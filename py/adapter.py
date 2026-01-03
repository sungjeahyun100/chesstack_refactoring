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
        self._last_move: Optional[Tuple[Tuple[int, int], Tuple[int, int]]] = None

    def _board_turn(self) -> str:
        """엔진(chessboard)의 턴 값을 문자열로 변환해 반환."""
        try:
            return COLOR_TO_STR.get(self._board.getTurn(), "none")
        except Exception:
            return "none"

    @property
    def turn(self) -> str:
        """현재 턴 색상 반환 ('white' or 'black')"""
        return self._board_turn()

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
                    "is_royal": getattr(p, "getIsRoyal", lambda: False)(),
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
        # 엔진 턴 상태에 동기화
        self._last_move = None

    def snapshot(self):
        """엔진 상태 스냅샷 (보드 + 턴 메타)"""
        return {
            "position": self._board.getPosition(),
            "last_move": self._last_move,
        }

    def restore(self, snap) -> None:
        """스냅샷으로 엔진 상태 복원"""
        self._board.setPosition(snap["position"])
        self._last_move = snap["last_move"]

    def owned_piece_at(self, file: int, rank: int) -> bool:
        """현재 턴 플레이어의 기물이 해당 위치에 있는지 확인"""
        p = self._board(file, rank)
        if p.getPieceType() == chess_ext.PieceType.NONE:
            return False
        ct = p.getColor()
        piece_color = COLOR_TO_STR.get(ct, "none")
        return piece_color == self.turn

    def legal_moves(self, file: int, rank: int) -> List[Tuple[int, int]]:
        """
        해당 위치 기물의 합법적 이동 목표 리스트 반환
        chess_ext의 calcLegalMovesInOnePiece 사용
        (현재 턴 색깔의 기물만 이동 가능)
        """
        # 현재 턴 색깔 확인
        try:
            piece = self._board(file, rank)
            piece_color = COLOR_TO_STR.get(piece.getColor(), "none")
            if piece_color != self.turn:
                return []  # 자신의 색깔이 아니면 이동 불가
        except Exception:
            return []
        try:
            mover_color = piece.getColor()
        except Exception:
            return []
        pgns = self._board.calcLegalMovesInOnePiece(mover_color, file, rank, False)
        targets = []

        for pgn in pgns:
            to_square = pgn.getToSquare()
            targets.append((to_square[0], to_square[1]))
        return targets

    def legal_placements(self, color: Optional[str] = None) -> List[Tuple[str, int, int]]:
        """
        착수 가능한 모든 (기물타입, file, rank) 리스트 반환
        color: 'white', 'black' 또는 None(현재 턴 색상)
        """
        if color is None:
            color = self.turn
        
        # calcLegalPlacePiece requires a ColorType parameter in C++ binding
        if color == "white":
            pgns = self._board.calcLegalPlacePiece(chess_ext.ColorType.WHITE)
        elif color == "black":
            pgns = self._board.calcLegalPlacePiece(chess_ext.ColorType.BLACK)
        else:
            pgns = []

        placements = []
        for pgn in pgns:
            pgn_color = COLOR_TO_STR.get(pgn.getColorType(), "none")
            if pgn_color != color:
                continue
            from_square = pgn.getFromSquare()
            piece_type = PIECE_TYPE_TO_STR.get(pgn.getPieceType(), "?")
            placements.append((piece_type, from_square[0], from_square[1]))
        return placements

    def legal_successions(self, color: Optional[str] = None) -> List[Tuple[int, int]]:
        """
        계승 가능한 모든 (file, rank) 리스트 반환
        color: 'white', 'black' 또는 None(현재 턴 색상)
        """
        if color is None:
            color = self.turn
        
        pgns = self._board.calcLegalSuccesion()
        successions = []
        for pgn in pgns:
            pgn_color = COLOR_TO_STR.get(pgn.getColorType(), "none")
            if pgn_color != color:
                continue
            from_square = pgn.getFromSquare()
            successions.append((from_square[0], from_square[1]))
        return successions

    def is_royal(self, file: int, rank: int) -> bool:
        """지정 칸 기물이 로얄인지 확인."""
        try:
            p = self._board(file, rank)
            return bool(p.getIsRoyal())
        except Exception:
            return False

    def disguise_options(self, file: int, rank: int) -> List[str]:
        """해당 로얄 기물이 선택할 수 있는 위장 후보 기물 심볼 목록 반환."""
        try:
            mover_color = self._board(file, rank).getColor()
        except Exception:
            return []
        try:
            pgns = self._board.calcLegalDisguise(mover_color)
        except Exception:
            return []

        options: List[str] = []
        for pgn in pgns:
            try:
                fs = pgn.getFromSquare()
                if fs[0] != file or fs[1] != rank:
                    continue
                sym = PIECE_TYPE_TO_STR.get(pgn.getPieceType(), "?")
                if sym and sym not in options:
                    options.append(sym)
            except Exception:
                continue
        return options

    def disguise(self, file: int, rank: int, piece_type_str: str) -> bool:
        """로얄 기물을 지정한 타입으로 위장."""
        pt = STR_TO_PIECE_TYPE.get(piece_type_str)
        if pt is None:
            return False
        try:
            mover_color = self._board(file, rank).getColor()
        except Exception:
            return False
        try:
            pgns = self._board.calcLegalDisguise(mover_color)
        except Exception:
            return False

        chosen = None
        for pgn in pgns:
            try:
                fs = pgn.getFromSquare()
                if fs[0] == file and fs[1] == rank and pgn.getPieceType() == pt:
                    chosen = pgn
                    break
            except Exception:
                continue

        if chosen is None:
            return False

        try:
            self._board.updatePiece(chosen)
        except Exception:
            return False
        self._last_move = ((file, rank), (file, rank))
        return True

    def move(self, src: Tuple[int, int], dst: Tuple[int, int]) -> bool:
        """
        기물 이동 시도
        반환: 성공 여부
        """
        sf, sr = src
        df, dr = dst
        try:
            # 합법적 이동 PGN 중에서 목적지가 일치하는 것 찾기
            try:
                mover_color = self._board(sf, sr).getColor()
            except Exception:
                return False
            legal_pgns = self._board.calcLegalMovesInOnePiece(mover_color, sf, sr, False)
            matching_pgn = None
            for pgn in legal_pgns:
                to_sq = pgn.getToSquare()
                to_f, to_r = to_sq
                if to_f == df and to_r == dr:
                    matching_pgn = pgn
                    break
            
            if matching_pgn is None:
                return False

            # 캡처 대상의 스택을 이동 전에 확보
            captured_stun = 0
            captured_move = 0
            try:
                mover_color = self._board(sf, sr).getColor()
                target_piece = self._board(df, dr)
                if target_piece.getPieceType() != chess_ext.PieceType.NONE and target_piece.getColor() != mover_color:
                    self._board.controllPocketValue(mover_color, target_piece.getPieceType(), 1)
                    captured_stun = target_piece.getStun()
                    captured_move = target_piece.getMove()
            except Exception:
                pass

            try:
                self._board.updatePiece(matching_pgn)
            except Exception:
                return False
            try:
                moved_piece = self._board(df, dr)
                if captured_stun:
                    moved_piece.addStun(captured_stun)
                if captured_move:
                    moved_piece.addMove(captured_move)
                moved_piece.minusOneMove()
            except Exception:
                pass
            self._last_move = (src, dst)
            return True
        except Exception:
            return False

    def promotion_options(self, src: Tuple[int, int], dst: Tuple[int, int]) -> List[str]:
        """
        src->dst가 프로모션일 경우 선택 가능한 승격 기물 심볼 리스트를 반환.
        """
        sf, sr = src
        df, dr = dst
        options: List[str] = []
        try:
            try:
                mover_color = self._board(sf, sr).getColor()
            except Exception:
                return []
            legal_pgns = self._board.calcLegalMovesInOnePiece(mover_color, sf, sr, False)
            for pgn in legal_pgns:
                to_sq = pgn.getToSquare()
                to_f, to_r = to_sq
                if to_f == df and to_r == dr:
                    # MoveType.PROMOTE인 경우만 수집
                    try:
                        if pgn.getMoveType() == chess_ext.MoveType.PROMOTE:
                            sym = PIECE_TYPE_TO_STR.get(pgn.getPieceType(), "?")
                            if sym and sym not in options:
                                options.append(sym)
                    except Exception:
                        # MoveType 바인딩이 없거나 접근 실패 시 무시
                        pass
        except Exception:
            pass
        return options

    def promote_move(self, src: Tuple[int, int], dst: Tuple[int, int], piece_type_str: str) -> bool:
        """
        선택한 승격 기물로 프로모션 이동 실행.
        """
        sf, sr = src
        df, dr = dst
        pt = STR_TO_PIECE_TYPE.get(piece_type_str)
        if pt is None:
            return False
        try:
            # 원본 스택
            try:
                origin_piece = self._board(sf, sr)
                old_stun = origin_piece.getStun()
                old_move = origin_piece.getMove()
            except Exception:
                old_stun = 0
                old_move = 0
            # 캡처 대상 스택
            captured_stun = 0
            captured_move = 0
            try:
                mover_color = self._board(sf, sr).getColor()
                target_piece = self._board(df, dr)
                if target_piece.getPieceType() != chess_ext.PieceType.NONE and target_piece.getColor() != mover_color:
                    captured_stun = target_piece.getStun()
                    captured_move = target_piece.getMove()
            except Exception:
                pass
            # 해당 승격 PGN 선택
            try:
                mover_color = self._board(sf, sr).getColor()
            except Exception:
                return False
            legal_pgns = self._board.calcLegalMovesInOnePiece(mover_color, sf, sr, False)
            chosen = None
            mover_color_type = self._board(sf, sr).getColor()
            
            for pgn in legal_pgns:
                to_f, to_r = pgn.getToSquare()
                if to_f == df and to_r == dr:
                    try:
                        # moveType, pieceType, 색상(colorType) 모두 확인
                        if (pgn.getMoveType() == chess_ext.MoveType.PROMOTE and 
                            pgn.getPieceType() == pt and 
                            pgn.getColorType() == mover_color_type):
                            chosen = pgn
                            break
                    except Exception:
                        pass
            if chosen is None:
                return False
            try:
                self._board.updatePiece(chosen)
            except Exception:
                return False
            try:
                promoted_piece = self._board(df, dr)
                promoted_piece.setStun(old_stun)
                promoted_piece.setMove(old_move)
                if captured_stun:
                    promoted_piece.addStun(captured_stun)
                if captured_move:
                    promoted_piece.addMove(captured_move)
                promoted_piece.minusOneMove()
            except Exception:
                pass
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
        ct = chess_ext.ColorType.WHITE if self.turn == "white" else chess_ext.ColorType.BLACK
        try:
            # ADD PGN 생성 (colorType, file, rank, pieceType)
            pgn = chess_ext.PGN(ct, file, rank, pt)
            try:
                self._board.updatePiece(pgn)
            except Exception:
                return False
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

    def succession(self, file: int, rank: int) -> bool:
        """
        해당 위치 기물을 왕위 계승 (로얄로 만들기)
        반환: 성공 여부
        """
        try:
            p = self._board(file, rank)
            if p.getPieceType() == chess_ext.PieceType.NONE:
                return False
            ct = p.getColor()
            # SUCCESION PGN 생성 (colorType, file, rank, moveType)
            pgn = chess_ext.PGN(ct, file, rank, chess_ext.MoveType.SUCCESION)
            try:
                self._board.updatePiece(pgn)
            except Exception:
                return False
            return True
        except Exception:
            return False

    def end_turn(self, flip: bool = False):
        """턴 종료 시 스택 처리. flip=True일 때만 턴을 넘긴다 (updatePiece를 거치지 않은 행동용)."""
        current = self.turn
        for f in range(8):
            for r in range(8):
                p = self._board(f, r)
                if p.getStun() != 0 and p.getColor() == STR_TO_COLOR_TYPE.get(current):
                    p.addStun(-1)
                    p.addMove(1)
        if flip:
            try:
                self._board.swapTurn()
            except Exception:
                pass

    def next_drop_kind(self, current: str) -> str:
        """드롭 기물 종류 순환"""
        kinds = [k for k in STR_TO_PIECE_TYPE.keys()]
        try:
            idx = kinds.index(current)
            return kinds[(idx + 1) % len(kinds)]
        except ValueError:
            return "P"

    def available_drop_kinds(self) -> List[str]:
        """현재 엔진에서 지원하는 드롭 기물 심볼 목록 반환 (NONE 제외)"""
        return [k for k in STR_TO_PIECE_TYPE.keys()]
