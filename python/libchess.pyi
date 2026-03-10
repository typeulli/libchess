"""
Type stubs for chesscpp — C++ Python extension module.

Color:   int  (WHITE=1, BLACK=0)
Square:  int  (A1=0 … H8=63)
Bitboard: int (64-bit unsigned)
PieceType: int (PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6)
"""

from __future__ import annotations
from typing import Optional

# ---------------------------------------------------------------------------
# Type aliases (all resolve to int at runtime)
# ---------------------------------------------------------------------------

Color = int      # WHITE=1, BLACK=0
Square = int     # A1=0 … H8=63
Bitboard = int   # 64-bit unsigned integer
PieceType = int  # PAWN=1 … KING=6

# ---------------------------------------------------------------------------
# Termination constants
# ---------------------------------------------------------------------------

CHECKMATE: int
STALEMATE: int
INSUFFICIENT_MATERIAL: int
SEVENTYFIVE_MOVES: int
FIVEFOLD_REPETITION: int
FIFTY_MOVES: int
THREEFOLD_REPETITION: int
VARIANT_WIN: int
VARIANT_LOSS: int
VARIANT_DRAW: int

# ---------------------------------------------------------------------------
# PieceType constants
# ---------------------------------------------------------------------------

PAWN: int    # = 1
KNIGHT: int  # = 2
BISHOP: int  # = 3
ROOK: int    # = 4
QUEEN: int   # = 5
KING: int    # = 6

PIECE_TYPES: list[int]                    # [1, 2, 3, 4, 5, 6]
PIECE_SYMBOLS: list[Optional[str]]        # [None, "p", "n", "b", "r", "q", "k"]
PIECE_NAMES: list[Optional[str]]          # [None, "pawn", "knight", "bishop", "rook", "queen", "king"]

# ---------------------------------------------------------------------------
# Color constants
# ---------------------------------------------------------------------------

WHITE: int  # = 1
BLACK: int  # = 0

COLORS: list[int]       # [1, 0]
COLOR_NAMES: list[str]  # ["black", "white"]  (indexed by color value)

# ---------------------------------------------------------------------------
# Square name / index lists
# ---------------------------------------------------------------------------

FILE_NAMES: list[str]   # ["a", "b", "c", "d", "e", "f", "g", "h"]
RANK_NAMES: list[str]   # ["1", "2", "3", "4", "5", "6", "7", "8"]

SQUARES: list[int]       # [0, 1, …, 63]
SQUARE_NAMES: list[str]  # ["a1", "b1", …, "h8"]
SQUARES_180: list[int]   # [square_mirror(sq) for sq in SQUARES]

STARTING_FEN: str        # "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
STARTING_BOARD_FEN: str  # "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

# ---------------------------------------------------------------------------
# Individual square integer constants  A1=0 … H8=63
# ---------------------------------------------------------------------------

A1: int; B1: int; C1: int; D1: int; E1: int; F1: int; G1: int; H1: int
A2: int; B2: int; C2: int; D2: int; E2: int; F2: int; G2: int; H2: int
A3: int; B3: int; C3: int; D3: int; E3: int; F3: int; G3: int; H3: int
A4: int; B4: int; C4: int; D4: int; E4: int; F4: int; G4: int; H4: int
A5: int; B5: int; C5: int; D5: int; E5: int; F5: int; G5: int; H5: int
A6: int; B6: int; C6: int; D6: int; E6: int; F6: int; G6: int; H6: int
A7: int; B7: int; C7: int; D7: int; E7: int; F7: int; G7: int; H7: int
A8: int; B8: int; C8: int; D8: int; E8: int; F8: int; G8: int; H8: int

# ---------------------------------------------------------------------------
# Bitboard constants
# ---------------------------------------------------------------------------

BB_EMPTY: int
BB_ALL: int
BB_LIGHT_SQUARES: int
BB_DARK_SQUARES: int
BB_CORNERS: int
BB_CENTER: int
BB_BACKRANKS: int

BB_SQUARES: list[int]  # BB_SQUARES[sq] = 1 << sq

BB_A1: int; BB_B1: int; BB_C1: int; BB_D1: int; BB_E1: int; BB_F1: int; BB_G1: int; BB_H1: int
BB_A2: int; BB_B2: int; BB_C2: int; BB_D2: int; BB_E2: int; BB_F2: int; BB_G2: int; BB_H2: int
BB_A3: int; BB_B3: int; BB_C3: int; BB_D3: int; BB_E3: int; BB_F3: int; BB_G3: int; BB_H3: int
BB_A4: int; BB_B4: int; BB_C4: int; BB_D4: int; BB_E4: int; BB_F4: int; BB_G4: int; BB_H4: int
BB_A5: int; BB_B5: int; BB_C5: int; BB_D5: int; BB_E5: int; BB_F5: int; BB_G5: int; BB_H5: int
BB_A6: int; BB_B6: int; BB_C6: int; BB_D6: int; BB_E6: int; BB_F6: int; BB_G6: int; BB_H6: int
BB_A7: int; BB_B7: int; BB_C7: int; BB_D7: int; BB_E7: int; BB_F7: int; BB_G7: int; BB_H7: int
BB_A8: int; BB_B8: int; BB_C8: int; BB_D8: int; BB_E8: int; BB_F8: int; BB_G8: int; BB_H8: int

BB_FILES: list[int]   # [BB_FILE_A, …, BB_FILE_H]
BB_FILE_A: int; BB_FILE_B: int; BB_FILE_C: int; BB_FILE_D: int
BB_FILE_E: int; BB_FILE_F: int; BB_FILE_G: int; BB_FILE_H: int

BB_RANKS: list[int]   # [BB_RANK_1, …, BB_RANK_8]
BB_RANK_1: int; BB_RANK_2: int; BB_RANK_3: int; BB_RANK_4: int
BB_RANK_5: int; BB_RANK_6: int; BB_RANK_7: int; BB_RANK_8: int

# ---------------------------------------------------------------------------
# Outcome
# ---------------------------------------------------------------------------

class Outcome:
    """Describes the result of a finished game."""

    @property
    def termination(self) -> int:
        """Termination constant (e.g. CHECKMATE, STALEMATE, …)."""
        ...

    @property
    def winner(self) -> Optional[int]:
        """Color int (WHITE=1 / BLACK=0) of the winner, or None for draws."""
        ...

    def __str__(self) -> str: ...

# ---------------------------------------------------------------------------
# Piece
# ---------------------------------------------------------------------------

class Piece:
    """A chess piece with a type and a color."""

    def __init__(self, piece_type: PieceType, color: Color) -> None: ...

    @property
    def type(self) -> PieceType:
        """PieceType int."""
        ...

    @property
    def piece_type(self) -> PieceType:
        """Alias for ``type``."""
        ...

    @property
    def color(self) -> Color:
        """Color int."""
        ...

    def __str__(self) -> str: ...

# ---------------------------------------------------------------------------
# Move
# ---------------------------------------------------------------------------

class Move:
    """A chess move (from_square → to_square, optional promotion / drop)."""

    def __init__(
        self,
        from_square: Square,
        to_square: Square,
        promotion: Optional[PieceType] = None,
    ) -> None: ...

    # ---- class method ----

    @staticmethod
    def from_uci(uci: str) -> Move:
        """Parse a UCI string (e.g. ``'e2e4'``, ``'e7e8q'``) into a Move.

        Raises ``ValueError`` on invalid input.
        """
        ...

    # ---- instance methods ----

    def uci(self) -> str:
        """Return the UCI string representation of the move."""
        ...

    # ---- properties ----

    @property
    def from_square(self) -> Square: ...

    @property
    def to_square(self) -> Square: ...

    @property
    def promotion(self) -> Optional[PieceType]:
        """Promotion piece type, or None."""
        ...

    @property
    def drop(self) -> Optional[PieceType]:
        """Drop piece type (used in Crazyhouse-style variants), or None."""
        ...

    def __str__(self) -> str: ...

# ---------------------------------------------------------------------------
# BaseBoard
# ---------------------------------------------------------------------------

class BaseBoard:
    """Low-level board that stores piece placement but no game-state (turn,
    castling rights, etc.)."""

    def __init__(self, board_fen: Optional[str] = None) -> None: ...

    # ---- bitboard properties ----

    @property
    def pawns(self) -> Bitboard: ...
    @property
    def knights(self) -> Bitboard: ...
    @property
    def bishops(self) -> Bitboard: ...
    @property
    def rooks(self) -> Bitboard: ...
    @property
    def queens(self) -> Bitboard: ...
    @property
    def kings(self) -> Bitboard: ...
    @property
    def occupied(self) -> Bitboard: ...

    # ---- methods ----

    def reset_board(self) -> None:
        """Reset piece placement to the standard starting position."""
        ...

    def clear_board(self) -> None:
        """Remove all pieces from the board."""
        ...

    def pieces_mask(self, piece_type: PieceType, color: Color) -> Bitboard:
        """Return the bitboard of all pieces of the given type and color."""
        ...

    def piece_at(self, square: Square) -> Optional[Piece]:
        """Return the piece on *square*, or ``None`` if the square is empty."""
        ...

    def piece_type_at(self, square: Square) -> Optional[PieceType]:
        """Return the piece type on *square*, or ``None``."""
        ...

    def color_at(self, square: Square) -> Optional[Color]:
        """Return the color of the piece on *square*, or ``None``."""
        ...

    def king(self, color: Color) -> Optional[Square]:
        """Return the square of the king of *color*, or ``None``."""
        ...

    def attacks_mask(self, square: Square) -> Bitboard:
        """Bitboard of squares attacked by the piece on *square*."""
        ...

    def attackers_mask(self, color: Color, square: Square) -> Bitboard:
        """Bitboard of *color* pieces that attack *square*."""
        ...

    def is_attacked_by(self, color: Color, square: Square) -> bool:
        """Return ``True`` if *square* is attacked by any piece of *color*."""
        ...

    def pin_mask(self, color: Color, square: Square) -> Bitboard:
        """Pin ray mask for the piece of *color* on *square*."""
        ...

    def is_pinned(self, color: Color, square: Square) -> bool:
        """Return ``True`` if the piece of *color* on *square* is pinned."""
        ...

    def remove_piece_at(self, square: Square) -> Optional[Piece]:
        """Remove and return the piece on *square*, or ``None``."""
        ...

    def set_piece_at(
        self,
        square: Square,
        piece: Optional[Piece],
        promoted: bool = False,
    ) -> None:
        """Place *piece* on *square* (pass ``None`` to clear it)."""
        ...

    def set_board_fen(self, fen: str) -> None:
        """Set piece placement from a board FEN string.

        Raises ``ValueError`` on invalid input.
        """
        ...

    def mirror(self) -> BaseBoard:
        """Return a vertically mirrored copy of the board."""
        ...

# ---------------------------------------------------------------------------
# Board
# ---------------------------------------------------------------------------

class Board(BaseBoard):
    """Full chess board including game-state (turn, castling rights, en
    passant, move stack, etc.)."""

    def __init__(self, fen: Optional[str] = None) -> None: ...

    # ---- game-state properties ----

    @property
    def turn(self) -> Color:
        """Side to move — WHITE (1) or BLACK (0)."""
        ...

    @property
    def fullmove_number(self) -> int:
        """Fullmove number (starts at 1, incremented after Black moves)."""
        ...

    @property
    def halfmove_clock(self) -> int:
        """Halfmove clock for the fifty-move rule."""
        ...

    @property
    def ep_square(self) -> Optional[Square]:
        """En passant target square, or ``None``."""
        ...

    @property
    def castling_rights(self) -> Bitboard:
        """Castling rights as a bitboard."""
        ...

    @property
    def legal_moves(self) -> list[Move]:
        """List of all currently legal moves."""
        ...

    @property
    def pseudo_legal_moves(self) -> list[Move]:
        """List of all pseudo-legal moves (ignoring checks)."""
        ...

    # ---- bitboard properties (inherited from BaseBoard, redeclared) ----

    @property
    def pawns(self) -> Bitboard: ...
    @property
    def knights(self) -> Bitboard: ...
    @property
    def bishops(self) -> Bitboard: ...
    @property
    def rooks(self) -> Bitboard: ...
    @property
    def queens(self) -> Bitboard: ...
    @property
    def kings(self) -> Bitboard: ...
    @property
    def occupied(self) -> Bitboard: ...

    # ---- board control ----

    def reset(self) -> None:
        """Reset to the standard starting position and clear the move stack."""
        ...

    def clear(self) -> None:
        """Clear the board and the move stack."""
        ...

    def clear_stack(self) -> None:
        """Clear only the move stack."""
        ...

    def copy(self) -> Board:
        """Return a deep copy of the board."""
        ...

    def mirror(self) -> Board:  # type: ignore[override]
        """Return a vertically mirrored copy (returns Board, not BaseBoard)."""
        ...

    # ---- move-stack ----

    def ply(self) -> int:
        """Number of half-moves played (length of move stack)."""
        ...

    def push(self, move: Move) -> None:
        """Apply *move* to the board and push it onto the move stack.

        Raises ``RuntimeError`` on invalid input.
        """
        ...

    def pop(self) -> Move:
        """Undo the last move and return it.

        Raises ``RuntimeError`` if the stack is empty.
        """
        ...

    def peek(self) -> Move:
        """Return the last move without undoing it.

        Raises ``RuntimeError`` if the stack is empty.
        """
        ...

    # ---- move generation ----

    def generate_legal_moves(
        self,
        from_mask: Bitboard = ...,
        to_mask: Bitboard = ...,
    ) -> list[Move]: ...

    def generate_pseudo_legal_moves(
        self,
        from_mask: Bitboard = ...,
        to_mask: Bitboard = ...,
    ) -> list[Move]: ...

    def generate_legal_captures(
        self,
        from_mask: Bitboard = ...,
        to_mask: Bitboard = ...,
    ) -> list[Move]: ...

    def generate_castling_moves(
        self,
        from_mask: Bitboard = ...,
        to_mask: Bitboard = ...,
    ) -> list[Move]: ...

    # ---- move classification ----

    def is_legal(self, move: Move) -> bool: ...
    def is_pseudo_legal(self, move: Move) -> bool: ...
    def is_capture(self, move: Move) -> bool: ...
    def is_en_passant(self, move: Move) -> bool: ...
    def is_castling(self, move: Move) -> bool: ...
    def is_kingside_castling(self, move: Move) -> bool: ...
    def is_queenside_castling(self, move: Move) -> bool: ...
    def gives_check(self, move: Move) -> bool: ...

    def find_move(
        self,
        from_square: Square,
        to_square: Square,
        promotion: Optional[PieceType] = None,
    ) -> Move:
        """Find a legal move matching the given squares / promotion.

        Raises ``ValueError`` if no such move exists.
        """
        ...

    # ---- game state queries ----

    def is_check(self) -> bool: ...
    def is_checkmate(self) -> bool: ...
    def is_stalemate(self) -> bool: ...
    def is_insufficient_material(self) -> bool: ...
    def is_game_over(self, claim_draw: bool = False) -> bool: ...
    def is_repetition(self, count: int = 3) -> bool: ...
    def is_seventyfive_moves(self) -> bool: ...
    def is_fivefold_repetition(self) -> bool: ...
    def is_fifty_moves(self) -> bool: ...
    def is_valid(self) -> bool: ...
    def was_into_check(self) -> bool: ...

    # ---- variant helpers ----

    def is_variant_end(self) -> bool: ...
    def is_variant_loss(self) -> bool: ...
    def is_variant_win(self) -> bool: ...
    def is_variant_draw(self) -> bool: ...
    def has_insufficient_material(self, color: Color) -> bool: ...

    # ---- draw-claim helpers ----

    def can_claim_draw(self) -> bool: ...
    def can_claim_fifty_moves(self) -> bool: ...
    def can_claim_threefold_repetition(self) -> bool: ...

    # ---- castling rights ----

    def has_castling_rights(self, color: Color) -> bool: ...
    def has_kingside_castling_rights(self, color: Color) -> bool: ...
    def has_queenside_castling_rights(self, color: Color) -> bool: ...

    # ---- result / outcome ----

    def result(self, claim_draw: bool = False) -> str:
        """Return the game result as a string (``"1-0"``, ``"0-1"``, ``"1/2-1/2"``, or ``"*"``)."""
        ...

    def outcome(self, claim_draw: bool = False) -> Optional[Outcome]:
        """Return an :class:`Outcome` if the game is over, otherwise ``None``."""
        ...

    def checkers_mask(self) -> Bitboard:
        """Bitboard of pieces that are currently giving check."""
        ...

    # ---- SAN / LAN / UCI notation ----

    def san(self, move: Move) -> str:
        """Return the SAN string for *move*."""
        ...

    def lan(self, move: Move) -> str:
        """Return the LAN (long algebraic notation) string for *move*."""
        ...

    def uci(self, move: Move) -> str:
        """Return the UCI string for *move* (board-context-aware)."""
        ...

    def parse_san(self, san: str) -> Move:
        """Parse a SAN string into a Move.

        Raises ``ValueError`` on invalid or ambiguous input.
        """
        ...

    def push_san(self, san: str) -> Move:
        """Parse and push a SAN move; returns the Move.

        Raises ``ValueError`` on invalid input.
        """
        ...

    def parse_uci(self, uci: str) -> Move:
        """Parse a UCI string in the context of this board.

        Raises ``ValueError`` on invalid input.
        """
        ...

    def push_uci(self, uci: str) -> Move:
        """Parse and push a UCI move; returns the Move.

        Raises ``ValueError`` on invalid input.
        """
        ...

    def variation_san(self, moves: list[Move]) -> str:
        """Return the SAN string for a sequence of moves from the current position."""
        ...

    # ---- FEN ----

    def fen(
        self,
        shredder: bool = False,
        en_passant: str = "legal",
    ) -> str:
        """Return the FEN string for the current position."""
        ...

    def set_fen(self, fen: str) -> None:
        """Set the board from a full FEN string.

        Raises ``ValueError`` on invalid input.
        """
        ...

    # ---- BaseBoard methods (re-declared for correct self type) ----

    def reset_board(self) -> None: ...
    def clear_board(self) -> None: ...
    def pieces_mask(self, piece_type: PieceType, color: Color) -> Bitboard: ...
    def piece_at(self, square: Square) -> Optional[Piece]: ...
    def piece_type_at(self, square: Square) -> Optional[PieceType]: ...
    def color_at(self, square: Square) -> Optional[Color]: ...
    def king(self, color: Color) -> Optional[Square]: ...
    def attacks_mask(self, square: Square) -> Bitboard: ...
    def attackers_mask(self, color: Color, square: Square) -> Bitboard: ...
    def is_attacked_by(self, color: Color, square: Square) -> bool: ...
    def pin_mask(self, color: Color, square: Square) -> Bitboard: ...
    def is_pinned(self, color: Color, square: Square) -> bool: ...
    def remove_piece_at(self, square: Square) -> Optional[Piece]: ...
    def set_piece_at(self, square: Square, piece: Optional[Piece], promoted: bool = False) -> None: ...
    def set_board_fen(self, fen: str) -> None: ...

# ---------------------------------------------------------------------------
# Module-level functions
# ---------------------------------------------------------------------------

# ---- square helpers ----

def parse_square(name: str) -> Square:
    """Parse a square name (e.g. ``"e4"``) and return its index (0-63)."""
    ...

def square(file_index: int, rank_index: int) -> Square:
    """Return the square index for the given file (0-7) and rank (0-7)."""
    ...

def square_at(file: int, rank: int) -> Square:
    """Alias for :func:`square`."""
    ...

def square_file(square: Square) -> int:
    """Return the file index (0-7) of *square*."""
    ...

def square_rank(square: Square) -> int:
    """Return the rank index (0-7) of *square*."""
    ...

def file(square: Square) -> int:  # noqa: A001
    """Return the file index (0-7) of *square*."""
    ...

def rank(square: Square) -> int:  # noqa: A001
    """Return the rank index (0-7) of *square*."""
    ...

def square_name(square: Square) -> str:
    """Return the name of *square* (e.g. ``"e4"``)."""
    ...

def square_mirror(square: Square) -> Square:
    """Return the vertically mirrored square (flip rank)."""
    ...

def mirror_square(square: Square) -> Square:
    """Alias for :func:`square_mirror`."""
    ...

def square_distance(a: Square, b: Square) -> int:
    """Chebyshev distance between squares *a* and *b*."""
    ...

def square_manhattan_distance(a: Square, b: Square) -> int:
    """Manhattan distance between squares *a* and *b*."""
    ...

def square_knight_distance(a: Square, b: Square) -> int:
    """Minimum knight moves needed to travel from *a* to *b*."""
    ...

# ---- piece helpers ----

def piece_symbol(piece_type: PieceType) -> str:
    """Return the single-character symbol for *piece_type* (e.g. ``"p"``)."""
    ...

def piece_name(piece_type: PieceType) -> str:
    """Return the full name for *piece_type* (e.g. ``"pawn"``)."""
    ...

# ---- bitboard helpers ----

def lsb(bitboard: Bitboard) -> int:
    """Return the index of the least significant set bit."""
    ...

def msb(bitboard: Bitboard) -> int:
    """Return the index of the most significant set bit."""
    ...

def scan_forward(bitboard: Bitboard) -> list[int]:
    """Return square indices for all set bits, LSB first."""
    ...

def scan_reversed(bitboard: Bitboard) -> list[int]:
    """Return square indices for all set bits, MSB first."""
    ...

def ray(from_sq: Square, to_sq: Square) -> Bitboard:
    """Return the ray bitboard from *from_sq* through *to_sq*."""
    ...

def between(from_sq: Square, to_sq: Square) -> Bitboard:
    """Return the bitboard of squares strictly between *from_sq* and *to_sq*."""
    ...

def flip_vertical(bb: Bitboard) -> Bitboard:
    """Flip the bitboard vertically (mirror across the middle rank)."""
    ...

def flip_horizontal(bb: Bitboard) -> Bitboard:
    """Flip the bitboard horizontally (mirror across the middle file)."""
    ...

def flip_diagonal(bb: Bitboard) -> Bitboard:
    """Flip the bitboard along the main diagonal (A1-H8)."""
    ...

def flip_anti_diagonal(bb: Bitboard) -> Bitboard:
    """Flip the bitboard along the anti-diagonal (A8-H1)."""
    ...
