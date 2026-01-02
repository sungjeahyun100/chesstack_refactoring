from __future__ import annotations

# UI 전역 상수 모음: 보드 크기, 패널 배치, 색상 팔레트, 글리프 매핑.
# 렌더와 입력 계산이 동일한 기준을 참조하도록 한 곳에서 관리한다.

# Board and layout sizes
BOARD_SIZE = 8
SQUARE = 80
BOARD_PX = BOARD_SIZE * SQUARE
INFO_W = 340
DEBUG_W = 260
ANALYSIS_H = 220

# Colors
LIGHT = (238, 238, 210)
DARK = (118, 150, 86)
SELECTED = (186, 202, 68)
TARGET = (244, 247, 116)
LAST = (246, 246, 105)
PANEL_BG = (32, 32, 32)
PANEL_TEXT = (235, 235, 235)
STUN_TEXT = (200, 50, 50)
BOT_ON_TEXT = (100, 200, 100)

# Piece glyph mapping
PIECE_GLYPH = {
    "K": "K", "Q": "Q", "R": "R", "B": "B", "N": "N", "P": "P",
    "A": "A", "G": "G", "Kr": "Kr", "W": "W", "D": "D", "L": "L",
    "F": "F", "C": "C", "Cl": "Cl", "Tr": "Tr",
}
