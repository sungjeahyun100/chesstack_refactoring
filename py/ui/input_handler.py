from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any, Callable, Dict, List, Tuple

import pygame

from ui.state import UIState

# 입력 처리: 키 매핑을 한곳에 정의해 play.py 루프를 단순화한다.

KeyHandler = Callable[["InputContext", pygame.event.Event], Tuple[bool, bool]]


@dataclass
class InputContext:
    engine: Any
    ui: UIState
    bot: Any
    friend_mode: bool
    mode_order: List[str]
    drop_kinds: List[str]
    create_bot: Callable[[Any, UIState], Any]

    def reset_ui(self):
        """엔진 상태를 유지하면서 UI/봇만 리셋."""
        self.engine.reset()
        self.ui = UIState()
        self.bot = self.create_bot(self.engine, self.ui)
        self.ui.bot_last_move_time = time.time()
        self.ui.analysis_dirty = True


def _handle_quit(ctx: InputContext, ev) -> Tuple[bool, bool]:
    return False, True


def _handle_reset(ctx: InputContext, ev) -> Tuple[bool, bool]:
    ctx.reset_ui()
    return True, False


def _handle_mode_set(mode: str) -> KeyHandler:
    def _inner(ctx: InputContext, ev) -> Tuple[bool, bool]:
        ctx.ui.mode = mode
        return True, False
    return _inner


def _handle_tab_cycle(ctx: InputContext, ev) -> Tuple[bool, bool]:
    ctx.ui.drop_kind = ctx.engine.next_drop_kind(ctx.ui.drop_kind)
    return True, False


def _handle_left(ctx: InputContext, ev) -> Tuple[bool, bool]:
    if ctx.ui.promoting and ctx.ui.promotion_choices:
        ctx.ui.promote_index = (ctx.ui.promote_index - 1) % len(ctx.ui.promotion_choices)
        ctx.ui.status = f"Promote: {ctx.ui.promotion_choices[ctx.ui.promote_index]}"
        return True, False

    if ctx.ui.focus == 'mode':
        try:
            i = ctx.mode_order.index(ctx.ui.mode)
            ctx.ui.mode = ctx.mode_order[(i - 1) % len(ctx.mode_order)]
        except ValueError:
            ctx.ui.mode = ctx.mode_order[0]
    elif ctx.ui.focus == 'drop' or ctx.ui.mode == 'drop':
        try:
            i = ctx.drop_kinds.index(ctx.ui.drop_kind)
            ctx.ui.drop_kind = ctx.drop_kinds[(i - 1) % len(ctx.drop_kinds)]
        except ValueError:
            ctx.ui.drop_kind = ctx.drop_kinds[0]
    else:
        try:
            i = ctx.mode_order.index(ctx.ui.mode)
            ctx.ui.mode = ctx.mode_order[(i - 1) % len(ctx.mode_order)]
        except ValueError:
            ctx.ui.mode = ctx.mode_order[0]
    return True, False


def _handle_right(ctx: InputContext, ev) -> Tuple[bool, bool]:
    if ctx.ui.promoting and ctx.ui.promotion_choices:
        ctx.ui.promote_index = (ctx.ui.promote_index + 1) % len(ctx.ui.promotion_choices)
        ctx.ui.status = f"Promote: {ctx.ui.promotion_choices[ctx.ui.promote_index]}"
        return True, False

    if ctx.ui.focus == 'mode':
        try:
            i = ctx.mode_order.index(ctx.ui.mode)
            ctx.ui.mode = ctx.mode_order[(i + 1) % len(ctx.mode_order)]
        except ValueError:
            ctx.ui.mode = ctx.mode_order[0]
    elif ctx.ui.focus == 'drop' or ctx.ui.mode == 'drop':
        try:
            i = ctx.drop_kinds.index(ctx.ui.drop_kind)
            ctx.ui.drop_kind = ctx.drop_kinds[(i + 1) % len(ctx.drop_kinds)]
        except ValueError:
            ctx.ui.drop_kind = ctx.drop_kinds[0]
    else:
        try:
            i = ctx.mode_order.index(ctx.ui.mode)
            ctx.ui.mode = ctx.mode_order[(i + 1) % len(ctx.mode_order)]
        except ValueError:
            ctx.ui.mode = ctx.mode_order[0]
    return True, False


def _handle_confirm(ctx: InputContext, ev) -> Tuple[bool, bool]:
    if ctx.ui.promoting and ctx.ui.promotion_choices and ctx.ui.promotion_from and ctx.ui.promotion_to:
        choice = ctx.ui.promotion_choices[ctx.ui.promote_index]
        ok = ctx.engine.promote_move(ctx.ui.promotion_from, ctx.ui.promotion_to, choice)
        if ok:
            ctx.engine.end_turn()
            ctx.ui.status = f"Promoted to {choice} (turn ended)"
            ctx.ui.analysis_dirty = True
        else:
            ctx.ui.status = "Promotion failed"
        ctx.ui.promoting = False
        ctx.ui.promotion_choices = []
        ctx.ui.promote_index = 0
        ctx.ui.promotion_from = None
        ctx.ui.promotion_to = None
        ctx.ui.selected = None
        ctx.ui.targets = []
    else:
        try:
            i = ctx.mode_order.index(ctx.ui.mode)
            ctx.ui.mode = ctx.mode_order[(i + 1) % len(ctx.mode_order)]
        except ValueError:
            ctx.ui.mode = ctx.mode_order[0]
    return True, False


def key_bindings() -> Dict[int, KeyHandler]:
    """키 매핑 정의."""
    return {
        pygame.K_ESCAPE: _handle_quit,
        pygame.K_q: _handle_quit,
        pygame.K_r: _handle_reset,
        pygame.K_m: _handle_mode_set("move"),
        pygame.K_d: _handle_mode_set("drop"),
        pygame.K_s: _handle_mode_set("stun"),
        pygame.K_TAB: _handle_tab_cycle,
        pygame.K_LEFT: _handle_left,
        pygame.K_RIGHT: _handle_right,
        pygame.K_RETURN: _handle_confirm,
        pygame.K_SPACE: _handle_confirm,
    }
