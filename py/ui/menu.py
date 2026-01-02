from __future__ import annotations
import pygame

# 초기 메뉴 화면을 담당하는 모듈.
# 화면에 버튼 3개(친구와 플레이, 봇과 플레이, 분석)와 오른쪽 타이틀을 보여주고
# 사용자 선택 결과를 문자열 키로 반환한다. (종료 시 None)


def show_menu(screen, info_font):
    """초기 메뉴 화면을 표시하고 선택 결과를 반환한다."""
    title_font = pygame.font.SysFont("arial", 56)
    sub_font = pygame.font.SysFont("arial", 24)

    btn_labels = [
        ("friend", "Play with Friends"),
        ("bot", "Play with Bot"),
        ("analysis", "Analysis"),
    ]
    btn_rects = []
    start_x, start_y = 40, 80
    bw, bh, gap = 260, 70, 18
    for i, _ in enumerate(btn_labels):
        rect = pygame.Rect(start_x, start_y + i * (bh + gap), bw, bh)
        btn_rects.append(rect)

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                return None
            if ev.type == pygame.KEYDOWN and ev.key in (pygame.K_ESCAPE, pygame.K_q):
                return None
            if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                for (key, _), rect in zip(btn_labels, btn_rects):
                    if rect.collidepoint(ev.pos):
                        return key

        screen.fill((18, 20, 26))

        mouse_pos = pygame.mouse.get_pos()

        # 왼쪽 버튼 컬럼
        for (_, label), rect in zip(btn_labels, btn_rects):
            hover = rect.collidepoint(mouse_pos)
            base_fill = (50, 70, 110)
            base_border = (140, 170, 230)
            fill_col = (230, 235, 245) if hover else base_fill
            border_col = base_fill if hover else base_border
            text_col = (40, 45, 60) if hover else (235, 235, 235)
            pygame.draw.rect(screen, fill_col, rect)
            pygame.draw.rect(screen, border_col, rect, 2)
            txt = info_font.render(label, True, text_col)
            screen.blit(txt, txt.get_rect(center=rect.center))

        # 오른쪽 제목 영역
        w, h = screen.get_size()
        title = title_font.render("chesstack", True, (235, 235, 245))
        subtitle = sub_font.render("Choose mode to start", True, (180, 180, 190))
        screen.blit(title, (w * 0.55, h * 0.25))
        screen.blit(subtitle, (w * 0.55, h * 0.25 + 60))

        pygame.display.flip()
        pygame.time.delay(16)


def show_bot_color_menu(screen, info_font):
    """봇 플레이 시 색상을 선택하는 간단한 2버튼 메뉴."""
    title_font = pygame.font.SysFont("arial", 40)
    btns = [("white", "Play as White"), ("black", "Play as Black")]
    bw, bh, gap = 240, 70, 18
    start_x = 40
    start_y = 120
    rects = [pygame.Rect(start_x, start_y + i * (bh + gap), bw, bh) for i in range(len(btns))]

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                return None
            if ev.type == pygame.KEYDOWN and ev.key in (pygame.K_ESCAPE, pygame.K_q):
                return None
            if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                for (key, _), rect in zip(btns, rects):
                    if rect.collidepoint(ev.pos):
                        return key

        screen.fill((18, 20, 26))

        mouse_pos = pygame.mouse.get_pos()

        title = title_font.render("Select Bot Color", True, (235, 235, 245))
        screen.blit(title, (40, 40))

        for (_, label), rect in zip(btns, rects):
            hover = rect.collidepoint(mouse_pos)
            base_fill = (60, 80, 120)
            base_border = (160, 190, 240)
            fill_col = (230, 235, 245) if hover else base_fill
            border_col = base_fill if hover else base_border
            text_col = (40, 45, 60) if hover else (235, 235, 235)
            pygame.draw.rect(screen, fill_col, rect)
            pygame.draw.rect(screen, border_col, rect, 2)
            txt = info_font.render(label, True, text_col)
            screen.blit(txt, txt.get_rect(center=rect.center))

        pygame.display.flip()
        pygame.time.delay(16)


def show_bot_type_menu(screen, info_font, options=None):
    """봇 타입을 선택하는 간단한 텍스트 버튼 메뉴."""
    title_font = pygame.font.SysFont("arial", 40)
    opts = options if options else ["my_propose", "GPTproposed"]
    bw, bh, gap = 240, 60, 14
    start_x = 40
    start_y = 120
    rects = [pygame.Rect(start_x, start_y + i * (bh + gap), bw, bh) for i in range(len(opts))]

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                return None
            if ev.type == pygame.KEYDOWN and ev.key in (pygame.K_ESCAPE, pygame.K_q):
                return None
            if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                for key, rect in zip(opts, rects):
                    if rect.collidepoint(ev.pos):
                        return key

        screen.fill((18, 20, 26))
        mouse_pos = pygame.mouse.get_pos()

        title = title_font.render("Select Bot Type", True, (235, 235, 245))
        screen.blit(title, (40, 40))

        for key, rect in zip(opts, rects):
            hover = rect.collidepoint(mouse_pos)
            base_fill = (70, 80, 110)
            base_border = (160, 190, 240)
            fill_col = (240, 220, 120) if hover else base_fill
            border_col = base_fill if hover else base_border
            text_col = (30, 25, 10) if hover else (235, 235, 235)
            pygame.draw.rect(screen, fill_col, rect)
            pygame.draw.rect(screen, border_col, rect, 2)
            txt = info_font.render(key, True, text_col)
            screen.blit(txt, txt.get_rect(center=rect.center))

        pygame.display.flip()
        pygame.time.delay(16)
