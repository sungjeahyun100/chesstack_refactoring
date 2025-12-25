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

	# calc legal moves for a pawn at e2 (index 4,1)
	moves = board.calcLegalMovesInOnePiece(4, 1)
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

if __name__ == '__main__':
	demo()