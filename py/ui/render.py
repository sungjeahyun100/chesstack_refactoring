from __future__ import annotations
import math
from typing import Optional, Tuple

import pygame

# 화면 렌더 전담 모듈: 보드/패널/분석 바를 그리고, 좌표 계산 헬퍼를 제공한다.

from ui.constants import (
    ANALYSIS_H,
    BOARD_PX,
    BOARD_SIZE,
    BOT_ON_TEXT,
    DEBUG_W,
    DARK,
    INFO_W,
    LAST,
    LIGHT,
    PANEL_BG,
    PANEL_TEXT,
    PIECE_GLYPH,
    SELECTED,
    SQUARE,
    STUN_TEXT,
    TARGET,
)
from ui.state import UIState


def board_from_mouse(pos: Tuple[int, int]) -> Optional[Tuple[int, int]]:
    """Mouse 픽셀 좌표를 보드 좌표(file, rank)로 변환."""
    mx, my = pos
    if mx >= BOARD_PX or my >= BOARD_PX:
        return None
    return mx // SQUARE, BOARD_SIZE - 1 - (my // SQUARE)


def layout_buttons(panel_width: int):
    dbg = pygame.Rect(BOARD_PX + 10, BOARD_PX - 60, panel_width - 20, 50)
    return dbg


def panel_click_rects(ui: UIState):
    """패널의 Mode/Drop 라인 클릭 영역을 계산."""
    panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
    x = BOARD_PX + 10
    # draw()에서 y 시작은 8, 각 라인은 20px 간격
    mode_y = 8 + 20 * 1
    drop_y = 8 + 20 * 2
    mode_rect = pygame.Rect(x, mode_y, panel_width - 20, 20)
    drop_rect = pygame.Rect(x, drop_y, panel_width - 20, 20)
    return mode_rect, drop_rect


def draw_board_and_pieces(engine, ui: UIState, screen, font, info_font):
    for f in range(BOARD_SIZE):
        for r in range(BOARD_SIZE):
            x, y = f * SQUARE, (BOARD_SIZE - 1 - r) * SQUARE
            base = LIGHT if (f + r) % 2 == 0 else DARK
            if ui.last_move and ((f, r) in ui.last_move):
                base = LAST
            if ui.selected == (f, r):
                base = SELECTED
            elif (f, r) in ui.targets:
                base = TARGET
            pygame.draw.rect(screen, base, (x, y, SQUARE, SQUARE))

    for p in engine.board():
        f, r = p["file"], p["rank"]
        x, y = f * SQUARE, (BOARD_SIZE - 1 - r) * SQUARE
        sym = PIECE_GLYPH.get(p["type"], str(p["type"]))
        if p["color"] == "black" and sym.isalpha():
            sym = sym.lower()
        text = font.render(sym, True, (20, 20, 20))
        rect = text.get_rect(center=(x + SQUARE // 2, y + SQUARE // 2))
        screen.blit(text, rect)
        if p.get("move_stack", 0) > 0:
            ms = info_font.render(str(p["move_stack"]), True, (100, 150, 255))
            screen.blit(ms, (x + 4, y + 4))
        if p.get("stun", 0) > 0:
            st = info_font.render(str(p["stun"]), True, STUN_TEXT)
            screen.blit(st, (x + SQUARE - 10 - st.get_width(), y + 4))
        if p.get("stunned"):
            mk = info_font.render("*", True, STUN_TEXT)
            screen.blit(mk, (x + SQUARE - 10 - mk.get_width(), y + SQUARE - 4 - mk.get_height()))


def draw_panel_background(screen, panel_width: int):
    panel = pygame.Rect(BOARD_PX, 0, panel_width, BOARD_PX)
    pygame.draw.rect(screen, PANEL_BG, panel)


def draw_header(engine, ui: UIState, screen, info_font, start_y: int) -> int:
    y = start_y
    surf = info_font.render(f"Turn: {engine.turn}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    mode_color = (255, 230, 120) if ui.focus == 'mode' else PANEL_TEXT
    surf = info_font.render(f"Mode: {ui.mode}", True, mode_color)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    drop_color = (255, 230, 120) if ui.focus == 'drop' else PANEL_TEXT
    surf = info_font.render(f"Drop: {ui.drop_kind}", True, drop_color)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    surf = info_font.render(f"Status: {ui.status}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    surf = info_font.render(f"Debug: {'ON' if ui.debug else 'OFF'}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    surf = info_font.render("", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    return y


def draw_pockets(engine, screen, info_font, start_y: int, mouse_pos=None):
    pocket_y = start_y
    wpocket = engine.pockets("white")
    bpocket = engine.pockets("black")
    surf = info_font.render("White Pocket:", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, pocket_y))
    wy = pocket_y + 20
    pocket_rects = []
    for pt, cnt in wpocket.items():
        rect = pygame.Rect(BOARD_PX + 10, wy - 2, 150, 20)
        hover = mouse_pos and rect.collidepoint(mouse_pos)
        bg = (70, 90, 130) if hover else (50, 70, 110)
        pygame.draw.rect(screen, bg, rect)
        pygame.draw.rect(screen, (140, 170, 230), rect, 1)
        surf = info_font.render(f"{pt}: {cnt}", True, (235, 235, 235) if not hover else (20, 20, 40))
        screen.blit(surf, (rect.x + 6, rect.y + 2))
        pocket_rects.append(("white", pt, rect))
        wy += 22
    surf = info_font.render("Black Pocket:", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 180, pocket_y))
    by = pocket_y + 20
    for pt, cnt in bpocket.items():
        rect = pygame.Rect(BOARD_PX + 180, by - 2, 150, 20)
        hover = mouse_pos and rect.collidepoint(mouse_pos)
        bg = (70, 90, 130) if hover else (50, 70, 110)
        pygame.draw.rect(screen, bg, rect)
        pygame.draw.rect(screen, (140, 170, 230), rect, 1)
        surf = info_font.render(f"{pt}: {cnt}", True, (235, 235, 235) if not hover else (20, 20, 40))
        screen.blit(surf, (rect.x + 6, rect.y + 2))
        pocket_rects.append(("black", pt, rect))
        by += 22
    return max(wy, by) + 10, pocket_rects


def draw_controls(screen, info_font, start_y: int, friend_mode: bool) -> int:
    y = start_y
    controls = [
        "Controls:",
        "  Click 'Mode' or 'Drop' to focus",
        "  Click pocket -> board to drop",
        "  Click piece -> target to move",
        "  Double-click piece: succession/stun menu",
        "  Double-click royal: disguise menu",
        "  R reset | Q/Esc quit",
    ]
    if friend_mode:
        # In friend mode, hide bot-specific hints implicitly by using the list above (no bot lines).
        pass
    for ln in controls:
        surf = info_font.render(ln, True, PANEL_TEXT)
        screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    return y


def draw_bot_box(ui: UIState, screen, info_font, panel_width: int, start_y: int, mouse_pos=None):
    box_x = BOARD_PX + 10
    box_y = start_y
    box_w = panel_width - 20
    box_h = 140
    pygame.draw.rect(screen, (40, 40, 60), (box_x, box_y, box_w, box_h))
    pygame.draw.rect(screen, (140, 140, 200), (box_x, box_y, box_w, box_h), 2)
    ty = box_y + 6
    surf = info_font.render("Bot Info", True, PANEL_TEXT)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    state_txt = "enabled" if ui.bot_enabled else "disabled"
    state_col = BOT_ON_TEXT if ui.bot_enabled else (140, 140, 140)
    surf = info_font.render(f"bot play mode: {state_txt}", True, state_col)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    bot_rects = []
    # Show current bot type (non-interactive)
    surf = info_font.render(f"Type: {ui.bot_type}", True, PANEL_TEXT)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    surf = info_font.render(f"Color: {ui.bot_color}", True, PANEL_TEXT)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    surf = info_font.render(f"Depth: {ui.depth}", True, PANEL_TEXT)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    surf = info_font.render(f"PlaceSample: {ui.placement_sample}", True, PANEL_TEXT)
    screen.blit(surf, (box_x + 8, ty)); ty += 18
    if ui.bot_move_str:
        surf = info_font.render(f"Last: {ui.bot_move_str}", True, (200, 200, 150))
        screen.blit(surf, (box_x + 8, ty)); ty += 18
    return box_y + box_h + 10, bot_rects


def draw_bottom_analysis(ui: UIState, screen, info_font, panel_width: int):
    total_w = BOARD_PX + panel_width
    bottom = pygame.Rect(0, BOARD_PX, total_w, ANALYSIS_H)
    pygame.draw.rect(screen, PANEL_BG, bottom)
    pygame.draw.rect(screen, (80, 80, 80), bottom, 1)

    ax = 10
    ay = BOARD_PX + 8
    eval_pawn = ui.analysis_eval / 100.0
    surf = info_font.render(f"Eval: {eval_pawn:+.2f} P", True, PANEL_TEXT)
    screen.blit(surf, (ax, ay))
    ay += 20

    best_txt = ui.analysis_best if ui.analysis_best else "(none)"
    surf = info_font.render(f"Best: {best_txt}", True, PANEL_TEXT)
    screen.blit(surf, (ax, ay))
    ay += 22

    bar_x = ax
    bar_w = total_w - 20
    bar_h = 16
    bar_y = ay
    scale = 2.0
    v = max(-20.0, min(20.0, eval_pawn))
    ratio_white = 0.5 * (1.0 + math.tanh(v / scale))
    split_x = bar_x + int(bar_w * ratio_white)

    pygame.draw.rect(screen, STUN_TEXT, (bar_x, bar_y, max(0, split_x - bar_x), bar_h))
    pygame.draw.rect(screen, BOT_ON_TEXT, (split_x, bar_y, max(0, bar_x + bar_w - split_x), bar_h))
    pygame.draw.line(screen, PANEL_TEXT, (split_x, bar_y), (split_x, bar_y + bar_h), 1)
    pygame.draw.rect(screen, PANEL_TEXT, (bar_x, bar_y, bar_w, bar_h), 1)
    ay += bar_h + 10

    surf = info_font.render("PV:", True, PANEL_TEXT)
    screen.blit(surf, (ax, ay))
    ay += 20
    if ui.analysis_pv:
        max_lines = max(1, (ANALYSIS_H - (ay - BOARD_PX) - 8) // 18)
        for ln in ui.analysis_pv[:max_lines]:
            surf = info_font.render(ln, True, PANEL_TEXT)
            screen.blit(surf, (ax, ay))
            ay += 18
    else:
        surf = info_font.render("(empty)", True, (140, 140, 140))
        screen.blit(surf, (ax, ay))


def draw_debug_button(screen, info_font, dbg_btn, ui: UIState):
    dbg_color = (80, 80, 120) if ui.debug else (60, 60, 60)
    pygame.draw.rect(screen, dbg_color, dbg_btn)
    pygame.draw.rect(screen, (140, 140, 200), dbg_btn, 2)
    dbg = info_font.render("DEBUG OVERLAY", True, (255, 255, 255))
    screen.blit(dbg, dbg.get_rect(center=dbg_btn.center))


def draw_debug_overlay(engine, ui: UIState, screen, info_font):
    if not (ui.debug and ui.hovered):
        return
    hp = next((p for p in engine.board() if p["file"] == ui.hovered[0] and p["rank"] == ui.hovered[1]), None)
    hy = 8
    for ln in ([f"Hover: {ui.hovered}"] + (
        [f"type={hp['type']} color={hp['color']}", f"stun={hp['stun']} ms={hp['move_stack']}"] if hp else ["empty"]
    )):
        surf = info_font.render(ln, True, (200, 220, 200))
        screen.blit(surf, (BOARD_PX + INFO_W + 10, hy)); hy += 18


def draw_promotion_overlay(ui: UIState, screen, info_font, panel_width: int):
    if not (ui.promoting and ui.promotion_choices):
        return
    area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 190, panel_width - 20, 80)
    pygame.draw.rect(screen, (40, 40, 60), area)
    pygame.draw.rect(screen, (140, 140, 200), area, 2)
    title = info_font.render("Select Promotion", True, (255, 255, 255))
    screen.blit(title, (area.x + 8, area.y + 6))
    x = area.x + 8
    y = area.y + 30
    box_w, box_h = 44, 28
    spacing = 8
    for i, sym in enumerate(ui.promotion_choices):
        rect = pygame.Rect(x, y, box_w, box_h)
        color = (200, 200, 80) if i == ui.promote_index else (100, 100, 100)
        pygame.draw.rect(screen, color, rect)
        pygame.draw.rect(screen, (220, 220, 220), rect, 2)
        t = info_font.render(sym, True, (20, 20, 20))
        screen.blit(t, t.get_rect(center=rect.center))
        x += box_w + spacing


def draw_disguise_overlay(ui: UIState, screen, info_font, panel_width: int):
    if not (ui.disguising and ui.disguise_choices):
        return
    area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 270, panel_width - 20, 80)
    pygame.draw.rect(screen, (45, 55, 70), area)
    pygame.draw.rect(screen, (150, 170, 220), area, 2)
    title = info_font.render("Select Disguise", True, (255, 255, 255))
    screen.blit(title, (area.x + 8, area.y + 6))
    x = area.x + 8
    y = area.y + 30
    box_w, box_h = 56, 28
    spacing = 8
    for i, sym in enumerate(ui.disguise_choices):
        rect = pygame.Rect(x, y, box_w, box_h)
        color = (200, 190, 120) if i == ui.disguise_index else (100, 110, 130)
        pygame.draw.rect(screen, color, rect)
        pygame.draw.rect(screen, (230, 230, 240), rect, 2)
        t = info_font.render(sym, True, (15, 15, 25))
        screen.blit(t, t.get_rect(center=rect.center))
        x += box_w + spacing


def draw_special_action_overlay(ui: UIState, screen, info_font, panel_width: int):
    if not ui.special_menu:
        return []
    area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 270, panel_width - 20, 80)
    pygame.draw.rect(screen, (55, 45, 65), area)
    pygame.draw.rect(screen, (180, 170, 220), area, 2)
    title = info_font.render("Choose action", True, (240, 240, 240))
    screen.blit(title, (area.x + 8, area.y + 6))
    x = area.x + 8
    y = area.y + 32
    box_w, box_h = 110, 28
    spacing = 10
    rects = []
    for opt in ui.special_options:
        rect = pygame.Rect(x, y, box_w, box_h)
        pygame.draw.rect(screen, (120, 110, 160), rect)
        pygame.draw.rect(screen, (210, 210, 240), rect, 2)
        t = info_font.render(opt.title(), True, (10, 10, 20))
        screen.blit(t, t.get_rect(center=rect.center))
        rects.append((opt, rect))
        x += box_w + spacing
    return rects


def draw_action_overlay(ui: UIState, screen, info_font, panel_width: int):
    """When a single target square has multiple candidate PGNs, show choices."""
    if not ui.action_menu or not ui.action_choices:
        return []
    area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 190, panel_width - 20, 80)
    pygame.draw.rect(screen, (40, 40, 60), area)
    pygame.draw.rect(screen, (140, 140, 200), area, 2)
    title = info_font.render("Choose Action", True, (255, 255, 255))
    screen.blit(title, (area.x + 8, area.y + 6))
    x = area.x + 8
    y = area.y + 30
    box_w, box_h = 200, 28
    spacing = 8
    rects = []
    for i, txt in enumerate(ui.action_choices):
        rect = pygame.Rect(x, y, box_w, box_h)
        color = (200, 180, 100) if i == ui.action_index else (100, 100, 100)
        pygame.draw.rect(screen, color, rect)
        pygame.draw.rect(screen, (220, 220, 240), rect, 2)
        t = info_font.render(txt, True, (20, 20, 20))
        screen.blit(t, t.get_rect(center=rect.center))
        rects.append((i, rect))
        x += box_w + spacing
        if x + box_w > area.x + area.w:
            x = area.x + 8
            y += box_h + 6
    return rects


def draw_victory_overlay(ui: UIState, screen, info_font, panel_width: int):
    """Game over popup. Visibility is controlled by ui.victory_visible flag."""
    if not ui.victory_visible:
        return

    total_w = BOARD_PX + panel_width
    view_h = screen.get_height()

    # Dim background
    dim = pygame.Surface((total_w, view_h), pygame.SRCALPHA)
    dim.fill((10, 10, 20, 170))
    screen.blit(dim, (0, 0))

    box_w, box_h = 360, 200
    box_x = (total_w - box_w) // 2
    box_y = (view_h - box_h) // 2
    box = pygame.Rect(box_x, box_y, box_w, box_h)

    pygame.draw.rect(screen, (60, 70, 100), box)
    pygame.draw.rect(screen, (210, 220, 255), box, 3)

    # Support explicit draw string
    if ui.victory_winner == "draw":
        title_txt = "Draw"
    else:
        title_txt = ui.victory_winner.title() + " wins!" if ui.victory_winner else "Game Over"
    reason_txt = ui.victory_reason if ui.victory_reason else ""

    title = info_font.render(title_txt, True, (255, 240, 200))
    screen.blit(title, title.get_rect(center=(box.centerx, box.y + 40)))

    if reason_txt:
        reason = info_font.render(reason_txt, True, (220, 220, 230))
        screen.blit(reason, reason.get_rect(center=(box.centerx, box.y + 80)))

    hint_lines = [
        "Press R to restart",
        "Press MENU/Q/Esc to leave",
    ]
    hy = box.y + 120
    for ln in hint_lines:
        surf = info_font.render(ln, True, (200, 210, 230))
        screen.blit(surf, surf.get_rect(center=(box.centerx, hy)))
        hy += 26


def draw(engine, ui: UIState, screen, font, info_font, mouse_pos=None, friend_mode: bool = False):
    """현재 엔진 상태와 UIState를 기준으로 전체 화면을 그린다."""
    panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
    dbg_btn = layout_buttons(panel_width) if not friend_mode else None

    draw_board_and_pieces(engine, ui, screen, font, info_font)
    draw_panel_background(screen, panel_width)
    y = draw_header(engine, ui, screen, info_font, start_y=8)
    y, pocket_rects = draw_pockets(engine, screen, info_font, start_y=y, mouse_pos=mouse_pos)
    y = draw_controls(screen, info_font, start_y=y, friend_mode=friend_mode)
    bot_rects = []
    if not friend_mode:
        y, bot_rects = draw_bot_box(ui, screen, info_font, panel_width, start_y=y, mouse_pos=mouse_pos)
        draw_bottom_analysis(ui, screen, info_font, panel_width)
    if not friend_mode and dbg_btn:
        draw_debug_button(screen, info_font, dbg_btn, ui)
    draw_debug_overlay(engine, ui, screen, info_font)
    draw_promotion_overlay(ui, screen, info_font, panel_width)
    draw_disguise_overlay(ui, screen, info_font, panel_width)
    action_rects = draw_action_overlay(ui, screen, info_font, panel_width)
    draw_victory_overlay(ui, screen, info_font, panel_width)
    special_rects = draw_special_action_overlay(ui, screen, info_font, panel_width)

    return dbg_btn, pocket_rects, special_rects, action_rects, bot_rects
