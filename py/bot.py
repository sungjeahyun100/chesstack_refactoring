#!/usr/bin/env python3
"""
간단한 체스 봇 AI
현재 모드에서 무작위로 합법적인 행동 선택
"""
from __future__ import annotations
import random
import copy
from typing import Tuple, Optional, List, Dict, Any
from adapter import ChessEngineAdapter


class SimpleBot:
    """합법적인 행동을 무작위로 선택하는 간단한 봇"""
    
    def __init__(self, engine: ChessEngineAdapter, color: str = "black"):
        """
        Args:
            engine: 체스 엔진 어댑터
            color: "white" 또는 "black"
        """
        self.engine = engine
        self.color = color
    
    def get_best_move(self) -> bool:
        """
        현재 턴에서 합법적인 행동 하나 선택해서 실행.
        성공하면 True, 실패하면 False 반환.
        """
        current_turn = self.engine.turn
        if current_turn != self.color:
            return False
        
        # 모드별로 합법적인 행동 수집
        actions = []
        
        # 이동 모드: 모든 합법적 이동
        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                for tf, tr in targets:
                    actions.append(("move", f, r, tf, tr))
        
        # 드롭 모드: 모든 합법적 드롭
        placements = self.engine.legal_placements(self.color)
        for piece_type, f, r in placements:
            actions.append(("drop", piece_type, f, r))
        
        # 승계 모드: 모든 합법적 승계 위치
        successions = self.engine.legal_successions(self.color)
        for f, r in successions:
            actions.append(("succession", f, r))
        
        # 합법적 행동이 없으면 False
        if not actions:
            return False
        
        # 무작위 행동 선택
        action = random.choice(actions)
        
        # 행동 실행
        if action[0] == "move":
            _, src_f, src_r, dst_f, dst_r = action
            if self.engine.move((src_f, src_r), (dst_f, dst_r)):
                # 프로모션 옵션 확인
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


class WeightedBot:
    """
    가중치 기반 봇: 이동을 먼저 선호, 그 다음 드롭, 그 다음 승계
    """
    
    def __init__(self, engine: ChessEngineAdapter, color: str = "black"):
        self.engine = engine
        self.color = color
    
    def get_best_move(self) -> bool:
        """
        이동을 먼저 시도, 없으면 드롭, 그 다음 승계
        """
        current_turn = self.engine.turn
        if current_turn != self.color:
            return False
        
        # 1. 이동 시도
        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                if targets:
                    tf, tr = random.choice(targets)
                    if self.engine.move((f, r), (tf, tr)):
                        # 프로모션 옵션 확인
                        promotion_opts = self.engine.promotion_options((f, r), (tf, tr))
                        if promotion_opts:
                            chosen_piece = "Q" if "Q" in promotion_opts else promotion_opts[0]
                            if self.engine.promote_move((f, r), (tf, tr), chosen_piece):
                                return True
                        else:
                            return True
        
        # 2. 드롭 시도
        placements = self.engine.legal_placements(self.color)
        if placements:
            piece_type, f, r = random.choice(placements)
            if self.engine.drop(piece_type, f, r):
                return True
        
        # 3. 승계 시도
        successions = self.engine.legal_successions(self.color)
        if successions:
            f, r = random.choice(successions)
            if self.engine.succession(f, r):
                return True
        
        return False


class NegamaxBot:
    """
    네가맥스 알고리즘을 사용한 체스 봇
    알파-베타 가지치기를 포함한 탐색
    """
    
    # 기물 가치 (체스 변형 기물 포함)
    PIECE_VALUES = {
        "K": 400,      # 킹
        "Q": 900,      # 퀸
        "R": 500,      # 룩
        "B": 330,      # 비숍
        "N": 320,      # 나이트
        "P": 100,      # 폰
        "A": 1300,     # 아마존 (Q+N)
        "G": 200,      # 그래스호퍼
        "Kr": 400,     # 나이트라이더
        "W": 650,      # 아크비숍 (B+N)
        "D": 250,      # 다바바
        "L": 250,      # 알필
        "F": 200,      # 퍼즈
        "C": 450,      # 센타우르
        "Cl": 300,     # 카멜
        "Tr": 600,     # 템페스트룩
        "": 0,
    }
    
    def __init__(self, engine: ChessEngineAdapter, color: str = "black", depth: int = 3):
        """
        Args:
            engine: 체스 엔진 어댑터
            color: "white" 또는 "black"
            depth: 탐색 깊이 (기본값 3)
        """
        self.engine = engine
        self.color = color
        self.depth = depth
        self.nodes_searched = 0
    
    def evaluate_board(self) -> float:
        """
        현재 보드 상태를 평가
        양수면 봇에게 유리, 음수면 불리
        """
        score = 0.0
        
        for piece in self.engine.board():
            piece_type = piece["type"]
            piece_color = piece["color"]
            
            # 기물 가치
            value = self.PIECE_VALUES.get(piece_type, 0)
            
            # 위치 보너스 (중앙 선호)
            file, rank = piece["file"], piece["rank"]
            center_bonus = 0
            if 2 <= file <= 5 and 2 <= rank <= 5:
                center_bonus = 10
            if 3 <= file <= 4 and 3 <= rank <= 4:
                center_bonus = 20
            
            # 스택 보너스
            stun_penalty = -piece.get("stun", 0) * 30  # 스턴은 불리
            move_bonus = piece.get("move_stack", 0) * 15  # 무브 스택은 유리
            
            total_value = value + center_bonus + stun_penalty + move_bonus
            
            # 내 기물이면 +, 상대 기물이면 -
            if piece_color == self.color:
                score += total_value
            else:
                score -= total_value
        
        # 포켓 평가 (드롭 가능한 기물)
        my_pocket = self.engine.pockets(self.color)
        enemy_pocket = self.engine.pockets("white" if self.color == "black" else "black")
        
        for piece_type, count in my_pocket.items():
            score += self.PIECE_VALUES.get(piece_type, 0) * count * 0.3
        
        for piece_type, count in enemy_pocket.items():
            score -= self.PIECE_VALUES.get(piece_type, 0) * count * 0.3
        
        return score
    
    def get_all_moves(self, color: str) -> List[Tuple]:
        """
        주어진 색상의 모든 합법적 행동 수집 (프로모션 포함)
        Returns: [ ("move", src, dst), ("promote", src, dst, piece), ("drop", piece_type, pos), ("succession", pos) ]
        """
        moves = []

        # 이동 / 프로모션 분리
        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                for tf, tr in targets:
                    promos = self.engine.promotion_options((f, r), (tf, tr))
                    if promos:
                        for sym in promos:
                            moves.append(("promote", (f, r), (tf, tr), sym))
                    else:
                        moves.append(("move", (f, r), (tf, tr)))

        # 드롭
        placements = self.engine.legal_placements(color)
        for piece_type, f, r in placements:
            moves.append(("drop", piece_type, (f, r)))

        # 승계
        successions = self.engine.legal_successions(color)
        for f, r in successions:
            moves.append(("succession", (f, r)))

        return moves

    def make_move(self, move: Tuple) -> bool:
        """행동 실행 + 턴 종료 처리"""
        ok = False
        if move[0] == "move":
            src, dst = move[1], move[2]  # type: ignore
            ok = self.engine.move(src, dst)
        elif move[0] == "promote":
            src, dst, sym = move[1], move[2], move[3]  # type: ignore
            if self.engine.move(src, dst):
                ok = self.engine.promote_move(src, dst, sym)
        elif move[0] == "drop":
            f, r = move[2]  # type: ignore
            ok = self.engine.drop(move[1], f, r)  # type: ignore
        elif move[0] == "succession":
            f, r = move[1]  # type: ignore
            ok = self.engine.succession(f, r)

        if ok:
            self.engine.end_turn()
        return ok

    def negamax(self, depth: int, alpha: float, beta: float, color_multiplier: int) -> float:
        """스냅샷 기반 네가맥스 (알파-베타)"""
        self.nodes_searched += 1

        if depth == 0:
            return color_multiplier * self.evaluate_board()

        current_color = self.engine.turn
        moves = self.get_all_moves(current_color)
        if not moves:
            return color_multiplier * self.evaluate_board()

        max_score = float('-inf')
        base_snap = self.engine.snapshot()

        for mv in moves:
            self.engine.restore(base_snap)
            if not self.make_move(mv):
                continue
            score = -self.negamax(depth - 1, -beta, -alpha, -color_multiplier)
            if score > max_score:
                max_score = score
            if score > alpha:
                alpha = score
            if alpha >= beta:
                break

        self.engine.restore(base_snap)
        return max_score
    
    def get_best_move(self) -> bool:
        """
        휴리스틱 평가로 최선의 수 찾아서 실행
        """
        current_turn = self.engine.turn
        if current_turn != self.color:
            return False
        
        self.nodes_searched = 0
        moves = []
        
        # 캡처 있는 이동을 먼저 수집
        capture_moves = []
        normal_moves = []
        
        for f in range(8):
            for r in range(8):
                targets = self.engine.legal_moves(f, r)
                for tf, tr in targets:
                    is_capture = False
                    for piece in self.engine.board():
                        if piece["file"] == tf and piece["rank"] == tr:
                            if piece["color"] != self.color:
                                is_capture = True
                                break
                    
                    if is_capture:
                        capture_moves.append(("move", (f, r), (tf, tr)))
                    else:
                        normal_moves.append(("move", (f, r), (tf, tr)))
        
        # 캡처 우선, 그 다음 일반 이동 제한
        moves.extend(capture_moves)
        moves.extend(normal_moves[:15])
        
        # 드롭
        placements = self.engine.legal_placements(self.color)
        for piece_type, f, r in placements[:5]:
            moves.append(("drop", piece_type, (f, r)))
        
        # 승계
        successions = self.engine.legal_successions(self.color)
        for f, r in successions[:3]:
            moves.append(("succession", (f, r)))
        
        if not moves:
            return False
        
        best_move = None
        best_score = float('-inf')
        
        for move in moves:
            score = self._evaluate_move(move)
            score += random.uniform(0, 0.5)
            
            if score > best_score:
                best_score = score
                best_move = move
        
        if best_move:
            return self.make_move(best_move)
        
        return False
    
    def _evaluate_move(self, move: Tuple) -> float:
        """행동 평가 (캡처 우선순위 높음)"""
        score = 0.0
        
        if move[0] == "move":
            src, dst = move[1], move[2]
            src_f, src_r = src
            dst_f, dst_r = dst
            
            # 캡처 보너스 (높음)
            for piece in self.engine.board():
                if piece["file"] == dst_f and piece["rank"] == dst_r:
                    if piece["color"] != self.color:
                        capture_value = self.PIECE_VALUES.get(piece["type"], 0)
                        score += capture_value * 2
                        
                        # 작은 기물로 큰 기물을 잡으면 추가 보너스
                        moving_piece_value = 0
                        for p in self.engine.board():
                            if p["file"] == src_f and p["rank"] == src_r:
                                moving_piece_value = self.PIECE_VALUES.get(p["type"], 0)
                                break
                        
                        if moving_piece_value < capture_value:
                            score += 100
                        break
            
            # 중앙 보너스
            center_dist_before = abs(src_f - 3.5) + abs(src_r - 3.5)
            center_dist_after = abs(dst_f - 3.5) + abs(dst_r - 3.5)
            score += (center_dist_before - center_dist_after) * 5
        
        elif move[0] == "drop":
            piece_type = move[1]
            score += self.PIECE_VALUES.get(piece_type, 0) * 0.3
        
        elif move[0] == "succession":
            score += 50
        
        return score


