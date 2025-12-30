#!/usr/bin/env python3
from __future__ import annotations
import pygame
from typing import Dict, List, Tuple, Optional
import time

from adapter import ChessEngineAdapter
from bot import MinimaxBot, MinimaxGPTBot
engine = ChessEngineAdapter()

# 필요한 어댑터 인터페이스 예시:
# adapter.turn -> "white"/"black"
# adapter.board() -> Iterable[Dict[file, rank, type, color, stun, move_stack, stunned, royal?, disguised_as?]]
# adapter.pockets(color) -> Dict[piece_type, count]
# adapter.last_move -> ((sx,sy),(dx,dy)) or None
# adapter.hover(pos) -> optional info dict (이미 board()로 충분하면 생략 가능)
# adapter.end_turn(), adapter.select(src,dst) 등은 이벤트 처리 시 호출 (여기서는 콜백으로 받음)

BOARD_SIZE = 8
SQUARE = 80
BOARD_PX = BOARD_SIZE * SQUARE
INFO_W = 340
DEBUG_W = 260
LIGHT = (238, 238, 210)
DARK = (118, 150, 86)
SELECTED = (186, 202, 68)
TARGET = (244, 247, 116)
LAST = (246, 246, 105)
PANEL_BG = (32, 32, 32)
PANEL_TEXT = (235, 235, 235)
STUN_TEXT = (200, 50, 50)

PIECE_GLYPH = {
    "K": "K", "Q": "Q", "R": "R", "B": "B", "N": "N", "P": "P",
    "A": "A", "G": "G", "Kr": "Kr", "W": "W", "D": "D", "L": "L",
    "F": "F", "C": "C", "Cl": "Cl","Tr": "Tr",
}

class UIState:
    def __init__(self):
        self.mode = "drop"
        self.drop_kind = "K"
        self.selected: Optional[Tuple[int,int]] = None
        self.targets: List[Tuple[int,int]] = []
        self.last_move: Optional[Tuple[Tuple[int,int], Tuple[int,int]]] = None
        self.status = "Ready"
        self.debug = True
        self.hovered: Optional[Tuple[int,int]] = None
        self.focus: Optional[str] = None  # 'mode' | 'drop' | None
        # Promotion selection state
        self.promoting: bool = False
        self.promotion_choices: List[str] = []
        self.promote_index: int = 0
        self.promotion_from: Optional[Tuple[int,int]] = None
        self.promotion_to: Optional[Tuple[int,int]] = None
        # Bot settings
        self.bot_enabled: bool = False  # 봇 활성화 여부
        self.bot_color: str = "black"   # 봇이 조종할 색 ("white" 또는 "black")
        self.bot_type: str = "my_propose" # 봇 타입 
        self.bot_last_move_time: float = 0
        # track which side the bot has already acted for this turn
        self.bot_acted_turn: Optional[str] = None
        # separate display string for last bot move (shown in its own panel area)
        self.bot_move_str: Optional[str] = None

def board_from_mouse(pos: Tuple[int,int]) -> Optional[Tuple[int,int]]:
    mx, my = pos
    if mx >= BOARD_PX: return None
    return mx // SQUARE, BOARD_SIZE - 1 - (my // SQUARE)

def layout_buttons(panel_width: int):
    dbg = pygame.Rect(BOARD_PX + 10, BOARD_PX - 120, panel_width - 20, 50)
    end = pygame.Rect(BOARD_PX + 10, BOARD_PX - 60, panel_width - 20, 50)
    return dbg, end

def panel_click_rects(ui: UIState):
    """패널의 Mode/Drop 라인 클릭 영역을 계산"""
    panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
    x = BOARD_PX + 10
    # draw()에서 y 시작은 8, 각 라인은 20px 간격
    mode_y = 8 + 20 * 1
    drop_y = 8 + 20 * 2
    mode_rect = pygame.Rect(x, mode_y, panel_width - 20, 20)
    drop_rect = pygame.Rect(x, drop_y, panel_width - 20, 20)
    return mode_rect, drop_rect

def draw(engine, ui: UIState, screen, font, info_font):
    panel_width = INFO_W + (DEBUG_W if ui.debug else 0)
    dbg_btn, end_btn = layout_buttons(panel_width)

    # board
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

    # pieces
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

    # panel
    panel = pygame.Rect(BOARD_PX, 0, panel_width, BOARD_PX)
    pygame.draw.rect(screen, PANEL_BG, panel)
    
    # 상단 정보
    # 상단 정보 라인 개별 렌더링 (포커스 강조)
    y = 8
    # Turn
    surf = info_font.render(f"Turn: {engine.turn}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    # Mode (focus highlight)
    mode_color = (255, 230, 120) if ui.focus == 'mode' else PANEL_TEXT
    surf = info_font.render(f"Mode: {ui.mode}", True, mode_color)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    # Drop (focus highlight)
    drop_color = (255, 230, 120) if ui.focus == 'drop' else PANEL_TEXT
    surf = info_font.render(f"Drop: {ui.drop_kind}", True, drop_color)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    # Status
    surf = info_font.render(f"Status: {ui.status}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    # Debug
    surf = info_font.render(f"Debug: {'ON' if ui.debug else 'OFF'}", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    # spacer
    surf = info_font.render("", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    
    # 포켓 정보 - 양쪽 배치
    pocket_y = y
    wpocket = engine.pockets("white")
    bpocket = engine.pockets("black")
    
    # White Pocket (왼쪽)
    surf = info_font.render("White Pocket:", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 10, pocket_y))
    wy = pocket_y + 20
    for pt, cnt in wpocket.items():
        surf = info_font.render(f"{pt}: {cnt}", True, PANEL_TEXT)
        screen.blit(surf, (BOARD_PX + 15, wy))
        wy += 18
    
    # Black Pocket (오른쪽)
    surf = info_font.render("Black Pocket:", True, PANEL_TEXT)
    screen.blit(surf, (BOARD_PX + 180, pocket_y))
    by = pocket_y + 20
    for pt, cnt in bpocket.items():
        surf = info_font.render(f"{pt}: {cnt}", True, PANEL_TEXT)
        screen.blit(surf, (BOARD_PX + 185, by))
        by += 18
    
    # Controls (포켓 아래)
    y = max(wy, by) + 10
    controls = [
        "Controls:",
        "  Click 'Mode' or 'Drop' to focus",
        "  Left/Right: Mode change (focus) | Drop kind (in Drop mode)",
        "  M move | D drop | S stun (direct)",
        "  Tab cycle drop (forward)",
        "  END TURN button",
        "  B toggle bot (black) | Shift+B bot (white)",
        "  N toggle bot type (weighted/negamax)",
        "  R reset | Q/Esc quit",
    ]
    for ln in controls:
        surf = info_font.render(ln, True, PANEL_TEXT)
        screen.blit(surf, (BOARD_PX + 10, y)); y += 20
    
    # Bot status
    if ui.bot_enabled:
        bot_text = f"Bot: {ui.bot_color} ({ui.bot_type})"
        surf = info_font.render(bot_text, True, (100, 200, 100))
    else:
        bot_text = "Bot: disabled"
        surf = info_font.render(bot_text, True, (100, 100, 100))
    screen.blit(surf, (BOARD_PX + 10, y))
    y += 20
    # Bot last-move display (separate from status)
    if ui.bot_move_str:
        surf = info_font.render(f"Bot Move: {ui.bot_move_str}", True, (200, 200, 150))
        screen.blit(surf, (BOARD_PX + 10, y))
        y += 20

    # buttons
    pygame.draw.rect(screen, (50,100,50), end_btn)
    pygame.draw.rect(screen, (100,200,100), end_btn, 2)
    btn = info_font.render("END TURN", True, (255,255,255))
    screen.blit(btn, btn.get_rect(center=end_btn.center))

    dbg_color = (80,80,120) if ui.debug else (60,60,60)
    pygame.draw.rect(screen, dbg_color, dbg_btn)
    pygame.draw.rect(screen, (140,140,200), dbg_btn, 2)
    dbg = info_font.render("DEBUG OVERLAY", True, (255,255,255))
    screen.blit(dbg, dbg.get_rect(center=dbg_btn.center))

    # debug hover
    if ui.debug and ui.hovered:
        hp = next((p for p in engine.board() if p["file"]==ui.hovered[0] and p["rank"]==ui.hovered[1]), None)
        hy = 8
        for ln in ([f"Hover: {ui.hovered}"] + (
            [f"type={hp['type']} color={hp['color']}", f"stun={hp['stun']} ms={hp['move_stack']}"] if hp else ["empty"]
        )):
            surf = info_font.render(ln, True, (200,220,200))
            screen.blit(surf, (BOARD_PX + INFO_W + 10, hy)); hy += 18

    # Promotion selection panel
    if ui.promoting and ui.promotion_choices:
        area = pygame.Rect(BOARD_PX + 10, BOARD_PX - 190, panel_width - 20, 80)
        pygame.draw.rect(screen, (40, 40, 60), area)
        pygame.draw.rect(screen, (140, 140, 200), area, 2)
        title = info_font.render("Select Promotion", True, (255,255,255))
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
            t = info_font.render(sym, True, (20,20,20))
            screen.blit(t, t.get_rect(center=rect.center))
            x += box_w + spacing

    return dbg_btn, end_btn

def main(engine):
    pygame.init()
    ui = UIState()
    screen = pygame.display.set_mode((BOARD_PX + INFO_W + DEBUG_W, BOARD_PX))
    pygame.display.set_caption("chesstack (testing)")
    font = pygame.font.SysFont("arial", SQUARE // 2)
    info_font = pygame.font.SysFont("arial", 18)
    clock = pygame.time.Clock()
    running = True
    MODE_ORDER = ["move", "drop", "succession", "stun"]
    DROP_KINDS = engine.available_drop_kinds()
    
    # 봇 초기화 함수
    def create_bot(color: str, bot_type: str):
        if bot_type == "GPTproposed":
            return MinimaxGPTBot(engine, color, depth=3)
        else:
            return MinimaxBot(engine, color, depth=3)
    
    # 봇 초기화
    bot = create_bot(ui.bot_color, ui.bot_type)
    ui.bot_last_move_time = time.time()

    while running:
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
                    bot = create_bot(ui.bot_color, ui.bot_type)
                    ui.bot_last_move_time = time.time()
                elif ev.key == pygame.K_n:
                    # N: toggle bot type
                    ui.bot_type = "GPTproposed" if ui.bot_type == "my_propose" else "my_propose"
                    bot = create_bot(ui.bot_color, ui.bot_type)
                    ui.status = f"Bot type: {ui.bot_type}"
                elif ev.key == pygame.K_b:
                    # B: toggle bot for black, Shift+B: toggle bot for white
                    if ev.mod & pygame.KMOD_SHIFT:
                        ui.bot_color = "white"
                    else:
                        ui.bot_color = "black"
                    ui.bot_enabled = not ui.bot_enabled
                    bot = create_bot(ui.bot_color, ui.bot_type)
                    ui.status = f"Bot {'enabled' if ui.bot_enabled else 'disabled'} for {ui.bot_color}"
                    ui.bot_last_move_time = time.time()
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
                        ui.status = f"Promoted to {choice}" if ok else "Promotion failed"
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
                dbg_btn,end_btn = layout_buttons(INFO_W + (DEBUG_W if ui.debug else 0))
                if dbg_btn.collidepoint(ev.pos):
                    ui.debug = not ui.debug
                    continue
                if end_btn.collidepoint(ev.pos):
                    engine.end_turn()
                    ui.status = "Turn passed"
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
                                ui.status = f"Promoted to {sym}" if ok else "Promotion failed"
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
                                    ui.status = "Moved" if ok else "Move failed"
                                    ui.selected = None
                                    ui.targets = []
                            else:
                                ui.status = "Not a legal target"
                    elif ui.mode == "drop":
                        ok = engine.drop(ui.drop_kind, x,y)
                        ui.status = "Dropped" if ok else "Drop failed"
                    elif ui.mode == "succession":
                        ok = engine.succession(x,y)
                        ui.status = "Succession" if ok else "Succession failed"
                    elif ui.mode == "stun":
                        ok = engine.stun(x,y)
                        ui.status = "Stun" if ok else "Stun failed"
        
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
        else:
            # reset act flag when it's not the bot's turn
            ui.bot_acted_turn = None

        screen.fill((0,0,0))
        dbg_btn, end_btn = draw(engine, ui, screen, font, info_font)
        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main(engine)



