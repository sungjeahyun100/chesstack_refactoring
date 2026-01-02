from __future__ import annotations
from typing import Callable, Dict

# 입력 처리 모듈 스켈레톤.
# 현재 이벤트 로직은 play.py에 있지만, 키/마우스 맵을 중앙화할 때 이 모듈로 옮긴다.

KeyHandler = Callable[[], None]
MouseHandler = Callable[[tuple[int, int]], None]


def key_bindings() -> Dict[int, KeyHandler]:
    """추후 키 매핑을 중앙화하기 위한 자리표시자."""
    return {}
