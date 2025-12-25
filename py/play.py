#!/usr/bin/env python3
from __future__ import annotations
import pygame
from typing import Dict, List, Tuple, Optional

from adapter import ChessEngineAdapter
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
        self.mode = "move"
        self.drop_kind = "P"
        self.selected: Optional[Tuple[int,int]] = None
        self.targets: List[Tuple[int,int]] = []
        self.last_move: Optional[Tuple[Tuple[int,int], Tuple[int,int]]] = None
        self.status = "Ready"
        self.debug = True
        self.hovered: Optional[Tuple[int,int]] = None

def board_from_mouse(pos: Tuple[int,int]) -> Optional[Tuple[int,int]]:
    mx, my = pos
    if mx >= BOARD_PX: return None
    return mx // SQUARE, BOARD_SIZE - 1 - (my // SQUARE)

def layout_buttons(panel_width: int):
    dbg = pygame.Rect(BOARD_PX + 10, BOARD_PX - 120, panel_width - 20, 50)
    end = pygame.Rect(BOARD_PX + 10, BOARD_PX - 60, panel_width - 20, 50)
    return dbg, end

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
    lines = [
        f"Turn: {engine.turn}",
        f"Mode: {ui.mode}",
        f"Drop: {ui.drop_kind}",
        f"Status: {ui.status}",
        f"Debug: {'ON' if ui.debug else 'OFF'}",
        "",
    ]
    y = 8
    for ln in lines:
        surf = info_font.render(ln, True, PANEL_TEXT)
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
        "  M move | D drop | S stun",
        "  Tab cycle drop",
        "  END TURN button",
        "  R reset | Q/Esc quit",
    ]
    for ln in controls:
        surf = info_font.render(ln, True, PANEL_TEXT)
        screen.blit(surf, (BOARD_PX + 10, y)); y += 20

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
                elif ev.key == pygame.K_m:
                    ui.mode = "move"
                elif ev.key == pygame.K_d:
                    ui.mode = "drop"
                elif ev.key == pygame.K_s:
                    ui.mode = "stun"
                elif ev.key == pygame.K_TAB:
                    ui.drop_kind = engine.next_drop_kind(ui.drop_kind)
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
                                ok = engine.move(ui.selected, (x,y))
                                ui.status = "Moved" if ok else "Move failed"
                                ui.selected = None
                                ui.targets = []
                            else:
                                ui.status = "Not a legal target"
                    elif ui.mode == "drop":
                        ok = engine.drop(ui.drop_kind, x,y)
                        ui.status = "Dropped" if ok else "Drop failed"
                    elif ui.mode == "stun":
                        ok = engine.stun(x,y)
                        ui.status = "Stun" if ok else "Stun failed"

        screen.fill((0,0,0))
        dbg_btn, end_btn = draw(engine, ui, screen, font, info_font)
        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main(engine)



