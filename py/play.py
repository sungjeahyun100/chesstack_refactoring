#!/usr/bin/env python3
from __future__ import annotations
import pygame
import time

from adapter import ChessEngineAdapter
from ui.bot_manager import create_bot, update_analysis
from ui.constants import ANALYSIS_H, BOARD_PX, DEBUG_W, INFO_W, SQUARE
from ui.input_handler import InputContext, key_bindings
from ui.menu import show_bot_color_menu, show_menu
from ui.render import board_from_mouse, draw, layout_buttons, panel_click_rects
from ui.state import UIState
from ui.menu import show_bot_type_menu

engine = ChessEngineAdapter()

def main(engine):
    pygame.init()
    screen = pygame.display.set_mode((BOARD_PX + INFO_W + DEBUG_W, BOARD_PX + ANALYSIS_H))
    pygame.display.set_caption("chesstack (testing)")
    font = pygame.font.SysFont("arial", SQUARE // 2)
    info_font = pygame.font.SysFont("arial", 18)
    clock = pygame.time.Clock()
    BOT_TYPES = ["my_propose", "GPTproposed"]
    MODE_ORDER = ["move", "drop", "succession", "stun"]
    DROP_KINDS = engine.available_drop_kinds()

    quit_game = False
    pocket_rects = []
    special_rects = []
    keymap = key_bindings()

    while True:
        engine.reset()
        ui = UIState()
        running = True
        friend_mode = False

        # Initial menu: choose friend/bot/analysis
        choice = show_menu(screen, info_font)
        if choice is None:
            break
        if choice == "friend":
            ui.bot_enabled = False
            ui.status = "Play with friends"
            friend_mode = True
        elif choice == "bot":
            friend_mode = False
            color_choice = show_bot_color_menu(screen, info_font)
            if color_choice is None:
                break
            # Player chooses their color; bot plays the opposite color
            ui.bot_color = "black" if color_choice == "white" else "white"
            type_choice = show_bot_type_menu(screen, info_font, BOT_TYPES)
            if type_choice is None:
                break
            ui.bot_type = type_choice
            ui.bot_enabled = True
            ui.status = f"Play as {color_choice} vs {ui.bot_type}"
        elif choice == "analysis":
            ui.bot_enabled = False
            type_choice = show_bot_type_menu(screen, info_font, BOT_TYPES)
            if type_choice is None:
                break
            ui.bot_type = type_choice
            ui.status = f"Analysis mode ({ui.bot_type})"
            friend_mode = False
        else:
            friend_mode = False
        
        # 화면 높이: 친구 모드는 하단 패널을 숨기므로 높이를 줄인다.
        view_h = BOARD_PX if friend_mode else (BOARD_PX + ANALYSIS_H)
        screen = pygame.display.set_mode((BOARD_PX + INFO_W + DEBUG_W, view_h))

        # 봇 초기화 (UI 설정 기반)
        bot = create_bot(engine, ui)
        ui.bot_last_move_time = time.time()
        ui.analysis_dirty = not friend_mode
        if not friend_mode:
            update_analysis(ui, bot)

        ctx = InputContext(engine, ui, bot, friend_mode, MODE_ORDER, DROP_KINDS, create_bot)

        go_menu = False

        while running:
            # 30 FPS 루프: 이벤트 처리 -> 봇 액션 -> 분석 갱신 -> 렌더
            clock.tick(30)
            panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
            dbg_baseline = BOARD_PX - 60
            menu_w, menu_h = 120, 32
            menu_btn = pygame.Rect(BOARD_PX + panel_width - 10 - menu_w, dbg_baseline - 8 - menu_h, menu_w, menu_h)
            # Bot type selector sits at the top-right of the side panel (only in bot/analysis modes)
            bot_btn = pygame.Rect(BOARD_PX + panel_width - 10 - 130, 10, 130, menu_h) if not friend_mode else None

            for ev in pygame.event.get():
                if ev.type == pygame.QUIT:
                    running = False
                    quit_game = True
                elif ev.type == pygame.KEYDOWN:
                    handler = keymap.get(ev.key)
                    if handler:
                        keep_running, quit_flag = handler(ctx, ev)
                        ui = ctx.ui
                        bot = ctx.bot
                        if quit_flag:
                            running = False
                            quit_game = True
                        running = running and keep_running
                        continue
                elif ev.type == pygame.MOUSEMOTION:
                    ui.hovered = board_from_mouse(ev.pos)
                elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                    now = pygame.time.get_ticks()
                    is_double = False
                    if ui.last_click_pos == ev.pos and (now - ui.last_click_time) < 350:
                        is_double = True
                    ui.last_click_time = now
                    ui.last_click_pos = ev.pos

                    mx, my = ev.pos
                    if menu_btn.collidepoint(ev.pos):
                        go_menu = True
                        running = False
                        break

                    if ui.victory_visible:
                        # After victory, only allow MENU/keys; ignore other clicks
                        continue

                    if bot_btn and bot_btn.collidepoint(ev.pos) and not friend_mode:
                        chosen = show_bot_type_menu(screen, info_font, BOT_TYPES)
                        if chosen:
                            ui.bot_type = chosen
                            bot = create_bot(engine, ui)
                            ui.status = f"Bot type: {ui.bot_type}"
                            ui.analysis_dirty = True
                        continue

                    # special action menu click
                    if ui.special_menu:
                        handled_special = False
                        for opt, rect in special_rects:
                            if rect.collidepoint(ev.pos):
                                if ui.special_square:
                                    sx, sy = ui.special_square
                                    if opt == "succession":
                                        ok = engine.succession(sx, sy)
                                        ui.status = "Succession (turn ended)" if ok else "Succession failed"
                                    elif opt == "stun":
                                        ok = engine.stun(sx, sy)
                                        ui.status = "Stun (turn ended)" if ok else "Stun failed"
                                    elif opt == "disguise":
                                        opts = engine.disguise_options(sx, sy)
                                        if opts:
                                            ui.disguising = True
                                            ui.disguise_choices = opts
                                            ui.disguise_index = 0
                                            ui.disguise_from = (sx, sy)
                                            ui.status = "Choose disguise type"
                                            ok = False  # opening overlay, not finishing turn yet
                                        else:
                                            ui.status = "No disguise options"
                                            ok = False
                                    else:
                                        ok = False
                                    if ok:
                                        # Stun does not pass through updatePiece, so we manually flip the turn
                                        engine.end_turn(flip=True)
                                        ui.analysis_dirty = True
                                    # Check victory after action
                                    if not ui.victory_visible:
                                        winner = engine.victory()
                                        if winner:
                                            ui.victory_visible = True
                                            ui.victory_winner = winner
                                            ui.victory_reason = "Engine reported victory"
                                            ui.status = f"{winner} wins"
                                ui.special_menu = False
                                ui.special_options = []
                                ui.special_square = None
                                handled_special = True
                                break
                        if handled_special:
                            continue

                    # disguise overlay click (royal)
                    if ui.disguising and ui.disguise_choices and ui.disguise_from:
                        panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
                        area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 270, panel_width - 20, 80)
                        if area.collidepoint(ev.pos):
                            x = area.x + 8
                            y = area.y + 30
                            box_w, box_h = 56, 28
                            spacing = 8
                            for i, sym in enumerate(ui.disguise_choices):
                                rect = pygame.Rect(x, y, box_w, box_h)
                                if rect.collidepoint(ev.pos):
                                    ui.disguise_index = i
                                    ok = engine.disguise(ui.disguise_from[0], ui.disguise_from[1], sym)
                                    if ok:
                                        engine.end_turn()
                                        ui.status = f"Disguised to {sym} (turn ended)"
                                        ui.analysis_dirty = True
                                    else:
                                        ui.status = "Disguise failed"
                                    ui.disguising = False
                                    ui.disguise_choices = []
                                    ui.disguise_from = None
                                    break
                                x += box_w + spacing
                            continue

                    if not friend_mode:
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

                    # pocket clicks: choose drop kind
                    hit_pocket = False
                    for color, pt, rect in pocket_rects:
                        if rect.collidepoint(ev.pos):
                            ui.pending_action = "drop"
                            ui.drop_kind = pt
                            ui.selected = None
                            ui.promoting = False
                            ui.status = f"Selected {pt} to drop"
                            hit_pocket = True
                            break
                    if hit_pocket:
                        continue

                    # Promotion choice click handling in panel area
                    if ui.promoting and ui.promotion_choices:
                        panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
                        area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 190, panel_width - 20, 80)
                        if area.collidepoint(ev.pos):
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
                        x, y = sq
                        # Double-click piece: open special action chooser; add disguise option for royal
                        if is_double and engine.owned_piece_at(x, y):
                            opts = ["succession", "stun"]
                            if engine.is_royal(x, y):
                                opts.append("disguise")
                            ui.disguising = False
                            ui.disguise_choices = []
                            ui.disguise_from = None
                            ui.special_menu = True
                            ui.special_square = (x, y)
                            ui.special_options = opts
                            ui.status = "Choose action"
                            continue

                        ui.disguising = False
                        ui.disguise_choices = []
                        ui.disguise_from = None
                        if ui.pending_action == "drop":
                            ok = engine.drop(ui.drop_kind, x, y)
                            if ok:
                                engine.end_turn()
                                ui.status = "Dropped (turn ended)"
                                ui.analysis_dirty = True
                            else:
                                ui.status = "Drop failed"
                            ui.pending_action = None
                        else:
                            if ui.selected is None:
                                if engine.owned_piece_at(x, y):
                                    ui.selected = (x, y)
                                    ui.targets = engine.legal_moves(x, y)
                                    ui.status = "Select target square"
                                else:
                                    ui.status = "Select your piece"
                            else:
                                if (x, y) == ui.selected:
                                    ui.selected = None
                                    ui.targets = []
                                elif (x, y) in ui.targets:
                                    opts = engine.promotion_options(ui.selected, (x, y))
                                    if opts:
                                        ui.promoting = True
                                        ui.promotion_choices = opts
                                        ui.promote_index = 0
                                        ui.promotion_from = ui.selected
                                        ui.promotion_to = (x, y)
                                        ui.status = f"Choose promotion ({', '.join(opts)})"
                                    else:
                                        ok = engine.move(ui.selected, (x, y))
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

                        # Check victory after player actions
                        if not ui.victory_visible:
                            winner = engine.victory()
                            if winner:
                                ui.victory_visible = True
                                ui.victory_winner = winner
                                ui.victory_reason = "Engine reported victory"
                                ui.status = f"{winner} wins"
            
            if not running:
                continue

            # 봇 턴 처리
            # Bot: act immediately when it's the bot's turn; act only once per turn
            if ui.bot_enabled and engine.turn == ui.bot_color and not ui.victory_visible:
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

            if ui.analysis_dirty and not friend_mode:
                update_analysis(ui, bot)
            if friend_mode:
                ui.analysis_dirty = False

            # Victory check after bot/action updates
            if not ui.victory_visible:
                winner = engine.victory()
                if winner:
                    ui.victory_visible = True
                    ui.victory_winner = winner
                    ui.victory_reason = "Engine reported victory"
                    ui.status = f"{winner} wins"

            screen.fill((0,0,0))
            dbg_btn, pocket_rects, special_rects, bot_rects = draw(engine, ui, screen, font, info_font, mouse_pos=pygame.mouse.get_pos(), friend_mode=friend_mode)
            # 메뉴 버튼 (우상단)
            hover_menu = menu_btn.collidepoint(pygame.mouse.get_pos())
            base_fill = (70, 70, 90)
            base_border = (160, 160, 200)
            fill_col = (235, 235, 245) if hover_menu else base_fill
            border_col = base_fill if hover_menu else base_border
            text_col = (40, 40, 60) if hover_menu else (235, 235, 235)

            pygame.draw.rect(screen, fill_col, menu_btn)
            pygame.draw.rect(screen, border_col, menu_btn, 2)
            mtxt = info_font.render("MENU", True, text_col)
            screen.blit(mtxt, mtxt.get_rect(center=menu_btn.center))

            # Bot type quick menu button (top-right)
            if bot_btn and not friend_mode:
                hover_bot = bot_btn.collidepoint(pygame.mouse.get_pos())
                base_fill_b = (70, 70, 90)
                base_border_b = (160, 160, 200)
                fill_b = (240, 210, 90) if hover_bot else base_fill_b
                border_b = base_fill_b if hover_bot else base_border_b
                text_b = (30, 25, 10) if hover_bot else (235, 235, 235)
                pygame.draw.rect(screen, fill_b, bot_btn)
                pygame.draw.rect(screen, border_b, bot_btn, 2)
                btxt = info_font.render("Bot Type", True, text_b)
                screen.blit(btxt, btxt.get_rect(center=bot_btn.center))

            pygame.display.flip()

            if quit_game:
                running = False

        if quit_game:
            break
        if go_menu:
            continue
        else:
            break

    pygame.quit()

if __name__ == "__main__":
    main(engine)



