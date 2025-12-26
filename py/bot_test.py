#!/usr/bin/env python3
"""
봇 사용 예제 및 테스트
"""
from adapter import ChessEngineAdapter
from bot import SimpleBot, WeightedBot, NegamaxBot


def test_bot():
    """간단한 봇 대 봇 게임 테스트"""
    engine = ChessEngineAdapter()
    
    white_bot = WeightedBot(engine, "white")
    black_bot = WeightedBot(engine, "black")
    
    max_turns = 50
    turn_count = 0
    
    print("Starting bot vs bot game...")
    print(f"Initial turn: {engine.turn}")
    
    while turn_count < max_turns:
        current_color = engine.turn
        bot = white_bot if current_color == "white" else black_bot
        
        print(f"\nTurn {turn_count + 1}: {current_color}")
        
        # 봇이 행동을 시도
        if bot.get_best_move():
            print(f"  {current_color} bot made a move")
        else:
            print(f"  {current_color} bot has no moves, ending turn")
            engine.end_turn()
        
        turn_count += 1
        
        # 간단한 보드 상태 출력
        pieces = list(engine.board())
        print(f"  Pieces on board: {len(pieces)}")
    
    print("\nGame finished!")
    print(f"Final turn: {engine.turn}")


def test_negamax_bot():
    """네가맥스 봇 대 WeightedBot 테스트"""
    engine = ChessEngineAdapter()
    
    # 네가맥스 봇 (흰색) vs 가중치 봇 (검은색)
    white_bot = NegamaxBot(engine, "white", depth=2)
    black_bot = WeightedBot(engine, "black")
    
    max_turns = 30
    turn_count = 0
    
    print("Starting Negamax bot (white) vs Weighted bot (black)...")
    print(f"Negamax depth: {white_bot.depth}")
    print(f"Initial turn: {engine.turn}\n")
    
    while turn_count < max_turns:
        current_color = engine.turn
        bot = white_bot if current_color == "white" else black_bot
        bot_name = "Negamax" if current_color == "white" else "Weighted"
        
        print(f"Turn {turn_count + 1}: {current_color} ({bot_name})")
        
        # 봇이 행동을 시도
        if bot.get_best_move():
            if isinstance(bot, NegamaxBot):
                print(f"  {bot_name} bot moved (nodes searched: {bot.nodes_searched})")
                print(f"  Board evaluation: {bot.evaluate_board():.1f}")
            else:
                print(f"  {bot_name} bot moved")
        else:
            print(f"  {bot_name} bot has no moves, ending turn")
            engine.end_turn()
        
        turn_count += 1
        
        # 간단한 보드 상태 출력
        pieces = list(engine.board())
        white_pieces = sum(1 for p in pieces if p["color"] == "white")
        black_pieces = sum(1 for p in pieces if p["color"] == "black")
        print(f"  Pieces: White {white_pieces}, Black {black_pieces}")
        print()
    
    print("Game finished!")
    print(f"Final turn: {engine.turn}")


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1 and sys.argv[1] == "negamax":
        test_negamax_bot()
    else:
        print("Available tests:")
        print("  python bot_test.py          - WeightedBot vs WeightedBot")
        print("  python bot_test.py negamax  - NegamaxBot vs WeightedBot")
        print()
        test_bot()
