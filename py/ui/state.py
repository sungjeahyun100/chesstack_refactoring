from __future__ import annotations
from dataclasses import dataclass, field
from typing import List, Optional, Tuple


@dataclass
class UIState:
    """런타임 UI 상태 집합: 입력 포커스, 프로모션, 봇/분석 설정을 단일 구조로 보관."""

    mode: str = "drop"
    drop_kind: str = "K"
    selected: Optional[Tuple[int, int]] = None
    targets: List[Tuple[int, int]] = field(default_factory=list)
    last_move: Optional[Tuple[Tuple[int, int], Tuple[int, int]]] = None
    status: str = "Ready"
    debug: bool = True
    hovered: Optional[Tuple[int, int]] = None
    focus: Optional[str] = None  # 'mode' | 'drop' | None

    # Promotion selection state
    promoting: bool = False
    promotion_choices: List[str] = field(default_factory=list)
    promote_index: int = 0
    promotion_from: Optional[Tuple[int, int]] = None
    promotion_to: Optional[Tuple[int, int]] = None

    # Disguise selection state (royal-only)
    disguising: bool = False
    disguise_choices: List[str] = field(default_factory=list)
    disguise_index: int = 0
    disguise_from: Optional[Tuple[int, int]] = None

    # Bot settings
    bot_enabled: bool = False
    bot_color: str = "black"  # "white" or "black"
    bot_type: str = "my_propose"  # default bot type
    placement_sample: int = 6
    bot_last_move_time: float = 0.0
    bot_acted_turn: Optional[str] = None
    bot_move_str: Optional[str] = None

    # Analysis display (PV + eval)
    analysis_eval: int = 0
    analysis_best: str = ""
    analysis_pv: List[str] = field(default_factory=list)
    depth: int = 6
    analysis_dirty: bool = True

    # Interaction helpers
    pending_action: Optional[str] = None  # 'drop'
    last_click_time: float = 0.0
    last_click_pos: Optional[Tuple[int, int]] = None
    special_menu: bool = False
    special_square: Optional[Tuple[int, int]] = None
    special_options: List[str] = field(default_factory=list)

    # Victory overlay (render-only; toggled externally)
    victory_visible: bool = False
    victory_winner: Optional[str] = None  # 'white' | 'black' | None
    victory_reason: str = ""
