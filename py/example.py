# Example usage of the chess_ext pybind11 extension
# This script will try to automatically add the project's build/ directory
# to PYTHONPATH so the built extension (chess_ext) can be imported whether
# you run the script from the repo root or from the py/ folder.
import os
import sys
import glob

# prepend build directory (project_root/build) to sys.path when present
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.normpath(os.path.join(script_dir, '..'))
build_dir = os.path.join(project_root, 'build')
if os.path.isdir(build_dir):
	sys.path.insert(0, build_dir)

try:
	import chess_ext #type: ignore
except ImportError:
	raise SystemExit("chess_ext module not found. Build the pybind11 extension first (set BUILD_PYBINDING=ON), or run with PYTHONPATH pointing to the build directory.")

# helpers: algebraic <-> indices
def alg_to_idx(s):
	file = ord(s[0].lower()) - ord('a')
	rank = int(s[1]) - 1
	return file, rank

def idx_to_alg(file, rank):
	return f"{chr(ord('a')+file)}{rank+1}"


def demo():
	board = chess_ext.ChessBoard()

	# place kings
	board.placePiece(chess_ext.ColorType.WHITE, chess_ext.PieceType.KING, *alg_to_idx('e1'))
	board.placePiece(chess_ext.ColorType.BLACK, chess_ext.PieceType.KING, *alg_to_idx('e8'))

	# place a column of white pawns on c1..c7 to show stun stacks
	for r in range(0, 7):
		board.placePiece(chess_ext.ColorType.WHITE, chess_ext.PieceType.PWAN, 2, r)

	# place a column of black pawns on g8..g2
	for r, rr in enumerate(range(7, 0, -1)):
		board.placePiece(chess_ext.ColorType.BLACK, chess_ext.PieceType.PWAN, 6, rr)

	print('\nInitial board:')
	board.displayBoard()

	# inspect piece at e1
	p_e1 = board(4, 0)  # uses operator() bound as __call__
	print(f"e1 piece type: {p_e1.getPieceType()} color: {p_e1.getColor()} stun: {p_e1.getStun()} move: {p_e1.getMove()}")

	# modify stacks from Python
	p_e1.setStun(0)
	p_e1.setMove(10)
	print('\nAfter modifying e1 stacks:')
	board.displayPieceAt(4, 0)

	# move king from e1 to e2
	board.movePiece(4, 0, 4, 1)
	print('\nAfter moving king e1->e2:')
	board.displayBoard()

	# calc legal moves for the piece at e2 (index 4,1)
	piece_at_e2 = board(4, 1)
	moves = board.calcLegalMovesInOnePiece(piece_at_e2.getColor(), 4, 1, False)
	print('\nLegal moves for piece at e2:')
	for m in moves:
		print(m)

	board.updatePiece(moves[0])
	board.displayBoard()
	print('\nLegal moves for piece at e2:')
	for m in moves:
		print(m)

	# show pockets
	board.displayPockets()

def demo_bot():
	"""Demonstrate using the Python-side `MinimaxBot` wrapper that calls
	into the C++ `chess_ext` Minimax implementation.
	"""
	# build/ on sys.path is already handled above; import adapter and bot
	try:
		from adapter import ChessEngineAdapter
		from bot import MinimaxBot
	except Exception as e:
		print("Could not import adapter or bot wrappers:", e)
		return

	# Create adapter and reuse the board constructed in demo() by re-creating
	# a small test position here for clarity.
	engine = ChessEngineAdapter()
	board = engine._board  # underlying chess_ext.ChessBoard

	# place kings for a legal position
	board.placePiece(chess_ext.ColorType.WHITE, chess_ext.PieceType.KING, *alg_to_idx('e1'))
	board.placePiece(chess_ext.ColorType.BLACK, chess_ext.PieceType.KING, *alg_to_idx('e8'))

	print('\nBoard before bot move:')
	board.displayBoard()

	# create a Minimax bot for white and ask it to play one move
	bot = MinimaxBot(engine, color='white', depth=10)

	# Example: request the principal variation (best line) from the bot
	# - `get_best_line(depth)` returns a list of PGN objects; we'll print
	#   a compact, human-readable representation.
	pv = bot.get_best_line(10)
	if pv:
		print('\nPrincipal variation (best line):')
		for i, mv in enumerate(pv, start=1):
			try:
				mt = mv.getMoveType()
				if mt == chess_ext.MoveType.MOVE or mt == chess_ext.MoveType.PROMOTE:
					sf, sr = mv.getFromSquare()
					df, dr = mv.getToSquare()
					print(f"{i}. {idx_to_alg(sf,sr)}->{idx_to_alg(df,dr)}")
				elif mt == chess_ext.MoveType.ADD:
					pt = mv.getPieceType()
					f, r = mv.getFromSquare()
					print(f"{i}. ADD piece_type={pt} @ {idx_to_alg(f,r)}")
				elif mt == chess_ext.MoveType.SUCCESION:
					f, r = mv.getFromSquare()
					print(f"{i}. SUCCESION @ {idx_to_alg(f,r)}")
				else:
					print(f"{i}. {mv}")
			except Exception:
				print(f"{i}. <invalid PGN>")
	else:
		print('\nNo principal variation returned by bot.get_best_line()')

	moved = bot.get_best_move()
	if moved:
		# the adapter's move functions do not automatically call end_turn(), so
		# call it to advance the turn and apply end-of-turn bookkeeping
		engine.end_turn()
		print('\nBot executed a move:')
		board.displayBoard()
	else:
		print('\nBot did not play a move (no legal moves or not its turn).')


def _pgn_to_human(mv):
	"""Small helper to print a PGN in a readable way."""
	mt = mv.getMoveType()
	if mt == chess_ext.MoveType.MOVE or mt == chess_ext.MoveType.PROMOTE:
		sf, sr = mv.getFromSquare()
		df, dr = mv.getToSquare()
		return f"{idx_to_alg(sf,sr)}->{idx_to_alg(df,dr)}"
	if mt == chess_ext.MoveType.ADD:
		pt = mv.getPieceType()
		f, r = mv.getFromSquare()
		return f"ADD {pt} @ {idx_to_alg(f,r)}"
	if mt == chess_ext.MoveType.SUCCESION:
		f, r = mv.getFromSquare()
		return f"SUCCESION @ {idx_to_alg(f,r)}"
	return repr(mv)


def demo_calc_info():
	"""Demonstrate the aggregate CalcInfo result from C++ minimax."""
	board = chess_ext.ChessBoard()

	# Minimal legal-ish position: kings + a pawn each
	board.placePiece(chess_ext.ColorType.WHITE, chess_ext.PieceType.KING, *alg_to_idx('e1'))
	board.placePiece(chess_ext.ColorType.BLACK, chess_ext.PieceType.KING, *alg_to_idx('e8'))
	board.placePiece(chess_ext.ColorType.WHITE, chess_ext.PieceType.PWAN, *alg_to_idx('e2'))
	board.placePiece(chess_ext.ColorType.BLACK, chess_ext.PieceType.PWAN, *alg_to_idx('e7'))

	print('\nBoard for getCalcInfo():')
	board.displayBoard()

	depth = 4

	bot = chess_ext.Minimax(chess_ext.ColorType.WHITE)
	bot.setFollowTurn(True)
	info = bot.getCalcInfo(board, depth)
	print(f"\n[Minimax] eval_val={info.eval_val} bestMove={_pgn_to_human(info.bestMove)}")
	if info.line:
		print('[Minimax] PV:')
		for i, mv in enumerate(info.line, start=1):
			print(f"  {i}. {_pgn_to_human(mv)}")
	else:
		print('[Minimax] PV is empty')

	bot2 = chess_ext.MinimaxGPT(chess_ext.ColorType.WHITE)
	# MinimaxGPT follows its internal minimax impl's follow_turn default (false).
	# To be robust, keep the turn consistent by using follow-turn behavior on the base minimax only.
	info2 = bot2.getCalcInfo(board, depth)
	print(f"\n[MinimaxGPT] eval_val={info2.eval_val} bestMove={_pgn_to_human(info2.bestMove)}")
	if info2.line:
		print('[MinimaxGPT] PV:')
		for i, mv in enumerate(info2.line, start=1):
			print(f"  {i}. {_pgn_to_human(mv)}")
	else:
		print('[MinimaxGPT] PV is empty')


if __name__ == '__main__':
	demo()
	demo_bot()
	demo_calc_info()