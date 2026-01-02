#!/usr/bin/env python3
from __future__ import annotations
import pygame
import time

from adapter import ChessEngineAdapter
from ui.bot_manager import create_bot, update_analysis
from ui.constants import ANALYSIS_H, BOARD_PX, DEBUG_W, INFO_W, SQUARE
from ui.render import board_from_mouse, draw, layout_buttons, panel_click_rects
from ui.state import UIState

engine = ChessEngineAdapter()

def main(engine):
    pygame.init()
    ui = UIState()
    screen = pygame.display.set_mode((BOARD_PX + INFO_W + DEBUG_W, BOARD_PX + ANALYSIS_H))
    pygame.display.set_caption("chesstack (testing)")
    font = pygame.font.SysFont("arial", SQUARE // 2)
    info_font = pygame.font.SysFont("arial", 18)
    clock = pygame.time.Clock()
    running = True
    MODE_ORDER = ["move", "drop", "succession", "stun"]
    DROP_KINDS = engine.available_drop_kinds()
    
    # 봇 초기화 (UI 설정 기반)
    bot = create_bot(engine, ui)
    ui.bot_last_move_time = time.time()
    ui.analysis_dirty = True
    # 시작 시 보드가 바뀌지 않아도 초기 평가/분석을 한 번 계산해 표시
    update_analysis(ui, bot)

    while running:
        # 30 FPS 루프: 이벤트 처리 -> 봇 액션 -> 분석 갱신 -> 렌더
        clock.tick(30)
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                running = False
            elif ev.type == pygame.KEYDOWN:
                if ev.key in (pygame.K_ESCAPE, pygame.K_q):
                    running = False
                elif ev.key == pygame.K_r:
                    engine.reset()
                    ui = UIState()
                    bot = create_bot(engine, ui)
                    ui.bot_last_move_time = time.time()
                    ui.analysis_dirty = True
                elif ev.key == pygame.K_n:
                    # N: toggle bot type
                    ui.bot_type = "GPTproposed" if ui.bot_type == "my_propose" else "my_propose"
                    bot = create_bot(engine, ui)
                    ui.status = f"Bot type: {ui.bot_type}"
                    ui.analysis_dirty = True
                elif ev.key == pygame.K_b:
                    # B: toggle bot for black, Shift+B: toggle bot for white
                    if ev.mod & pygame.KMOD_SHIFT:
                        ui.bot_color = "white"
                    else:
                        ui.bot_color = "black"
                    ui.bot_enabled = not ui.bot_enabled
                    bot = create_bot(engine, ui)
                    ui.status = f"Bot {'enabled' if ui.bot_enabled else 'disabled'} for {ui.bot_color}"
                    ui.bot_last_move_time = time.time()
                    ui.analysis_dirty = True
                elif ev.key == pygame.K_m:
                    ui.mode = "move"
                elif ev.key == pygame.K_d:
                    ui.mode = "drop"
                elif ev.key == pygame.K_s:
                    ui.mode = "stun"
                elif ev.key == pygame.K_TAB:
                    ui.drop_kind = engine.next_drop_kind(ui.drop_kind)
                elif ev.key == pygame.K_LEFT:
                    # Promotion selection active: cycle choice
                    if ui.promoting and ui.promotion_choices:
                        ui.promote_index = (ui.promote_index - 1) % len(ui.promotion_choices)
                        ui.status = f"Promote: {ui.promotion_choices[ui.promote_index]}"
                        continue
                    # 좌우 화살표: 모드 포커스 시 모드 변경, 드롭 포커스 또는 드롭 모드일 때 드롭 종류 변경
                    if ui.focus == 'mode':
                        try:
                            i = MODE_ORDER.index(ui.mode)
                            ui.mode = MODE_ORDER[(i - 1) % len(MODE_ORDER)]
                        except ValueError:
                            ui.mode = MODE_ORDER[0]
                    elif ui.focus == 'drop' or ui.mode == 'drop':
                        try:
                            i = DROP_KINDS.index(ui.drop_kind)
                            ui.drop_kind = DROP_KINDS[(i - 1) % len(DROP_KINDS)]
                        except ValueError:
                            ui.drop_kind = DROP_KINDS[0]
                    else:
                        try:
                            i = MODE_ORDER.index(ui.mode)
                            ui.mode = MODE_ORDER[(i - 1) % len(MODE_ORDER)]
                        except ValueError:
                            ui.mode = MODE_ORDER[0]
                elif ev.key == pygame.K_RIGHT:
                    if ui.promoting and ui.promotion_choices:
                        ui.promote_index = (ui.promote_index + 1) % len(ui.promotion_choices)
                        ui.status = f"Promote: {ui.promotion_choices[ui.promote_index]}"
                        continue
                    if ui.focus == 'mode':
                        try:
                            i = MODE_ORDER.index(ui.mode)
                            ui.mode = MODE_ORDER[(i + 1) % len(MODE_ORDER)]
                        except ValueError:
                            ui.mode = MODE_ORDER[0]
                    elif ui.focus == 'drop' or ui.mode == 'drop':
                        try:
                            i = DROP_KINDS.index(ui.drop_kind)
                            ui.drop_kind = DROP_KINDS[(i + 1) % len(DROP_KINDS)]
                        except ValueError:
                            ui.drop_kind = DROP_KINDS[0]
                elif ev.key in (pygame.K_RETURN, pygame.K_SPACE):
                    # Confirm promotion selection
                    if ui.promoting and ui.promotion_choices and ui.promotion_from and ui.promotion_to:
                        choice = ui.promotion_choices[ui.promote_index]
                        ok = engine.promote_move(ui.promotion_from, ui.promotion_to, choice)
                        if ok:
                            engine.end_turn()
                            ui.status = f"Promoted to {choice} (turn ended)"
                            ui.analysis_dirty = True
                        else:
                            ui.status = "Promotion failed"
                        ui.promoting = False
                        ui.promotion_choices = []
                        ui.promote_index = 0
                        ui.promotion_from = None
                        ui.promotion_to = None
                        ui.selected = None
                        ui.targets = []
                    else:
                        try:
                            i = MODE_ORDER.index(ui.mode)
                            ui.mode = MODE_ORDER[(i + 1) % len(MODE_ORDER)]
                        except ValueError:
                            ui.mode = MODE_ORDER[0]
                # Up/Down arrows removed per request
            elif ev.type == pygame.MOUSEMOTION:
                ui.hovered = board_from_mouse(ev.pos)
            elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                mx,my = ev.pos
                dbg_btn = layout_buttons(INFO_W + (DEBUG_W if ui.debug else 0))
                if dbg_btn.collidepoint(ev.pos):
                    ui.debug = not ui.debug
                    continue
                # 패널 라인 클릭 포커스 설정
                mode_rect, drop_rect = panel_click_rects(ui)
                if mode_rect.collidepoint(ev.pos):
                    ui.focus = 'mode'
                    ui.status = "Focus: Mode"
                    continue
                if drop_rect.collidepoint(ev.pos):
                    ui.focus = 'drop'
                    ui.status = "Focus: Drop"
                    continue
                # Promotion choice click handling in panel area
                if ui.promoting and ui.promotion_choices:
                    panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
                    area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 190, panel_width - 20, 80)
                    if area.collidepoint(ev.pos):
                        # Compute rects and check which is clicked
                        x = area.x + 8
                        y = area.y + 30
                        box_w, box_h = 44, 28
                        spacing = 8
                        for i, sym in enumerate(ui.promotion_choices):
                            rect = pygame.Rect(x, y, box_w, box_h)
                            if rect.collidepoint(ev.pos):
                                ui.promote_index = i
                                ok = engine.promote_move(ui.promotion_from, ui.promotion_to, sym) if (ui.promotion_from and ui.promotion_to) else False
                                if ok:
                                    engine.end_turn()
                                    ui.status = f"Promoted to {sym} (turn ended)"
                                    ui.analysis_dirty = True
                                else:
                                    ui.status = "Promotion failed"
                                ui.promoting = False
                                ui.promotion_choices = []
                                ui.promote_index = 0
                                ui.promotion_from = None
                                ui.promotion_to = None
                                ui.selected = None
                                ui.targets = []
                                break
                            x += box_w + spacing
                        continue
                sq = board_from_mouse(ev.pos)
                if sq:
                    x,y = sq
                    if ui.mode == "move":
                        if ui.selected is None:
                            if engine.owned_piece_at(x,y):
                                ui.selected = (x,y)
                                ui.targets = engine.legal_moves(x,y)
                            else:
                                ui.status = "Select your piece"
                        else:
                            if (x,y) == ui.selected:
                                ui.selected = None
                                ui.targets = []
                            elif (x,y) in ui.targets:
                                # Check promotion options before moving
                                opts = engine.promotion_options(ui.selected, (x,y))
                                if opts:
                                    ui.promoting = True
                                    ui.promotion_choices = opts
                                    ui.promote_index = 0
                                    ui.promotion_from = ui.selected
                                    ui.promotion_to = (x,y)
                                    ui.status = f"Choose promotion ({', '.join(opts)})"
                                else:
                                    ok = engine.move(ui.selected, (x,y))
                                    if ok:
                                        engine.end_turn()
                                        ui.status = "Moved (turn ended)"
                                        ui.analysis_dirty = True
                                    else:
                                        ui.status = "Move failed"
                                    ui.selected = None
                                    ui.targets = []
                            else:
                                ui.status = "Not a legal target"
                    elif ui.mode == "drop":
                        ok = engine.drop(ui.drop_kind, x,y)
                        if ok:
                            engine.end_turn()
                            ui.status = "Dropped (turn ended)"
                            ui.analysis_dirty = True
                        else:
                            ui.status = "Drop failed"
                    elif ui.mode == "succession":
                        ok = engine.succession(x,y)
                        if ok:
                            engine.end_turn()
                            ui.status = "Succession (turn ended)"
                            ui.analysis_dirty = True
                        else:
                            ui.status = "Succession failed"
                    elif ui.mode == "stun":
                        ok = engine.stun(x,y)
                        if ok:
                            engine.end_turn()
                            ui.status = "Stun (turn ended)"
                            ui.analysis_dirty = True
                        else:
                            ui.status = "Stun failed"
        
        # 봇 턴 처리
        # Bot: act immediately when it's the bot's turn; act only once per turn
        if ui.bot_enabled and engine.turn == ui.bot_color:
            if ui.bot_acted_turn != engine.turn:
                moved = bot.get_best_move()
                # prefer adapter-provided standardized move string (set by bot wrapper on engine)
                mvstr = bot._last_move_str 
                if moved:
                    ui.bot_move_str = mvstr if mvstr else f"({ui.bot_color}/{ui.bot_type}) moved"
                else:
                    ui.bot_move_str = None
                # end the turn only after the bot call returns
                engine.end_turn()
                ui.selected = None
                ui.targets = []
                ui.bot_acted_turn = engine.turn
                ui.analysis_dirty = True
        else:
            # reset act flag when it's not the bot's turn
            ui.bot_acted_turn = None

        if ui.analysis_dirty:
            update_analysis(ui, bot)

        screen.fill((0,0,0))
        dbg_btn = draw(engine, ui, screen, font, info_font)
        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main(engine)



