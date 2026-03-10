#define PY_SSIZE_T_CLEAN
#include <memory>
#include <optional>
#include <string>
#include <Python.h>
#include "../src/libchess.hpp"

using namespace chess;

// ============================================================
// Helpers
// ============================================================

static std::optional<PieceType> pyobj_to_optional_piece_type(PyObject* obj) {
    if (obj == Py_None) return std::nullopt;
    long v = PyLong_AsLong(obj);
    if (v == -1 && PyErr_Occurred()) return std::nullopt;
    return (PieceType)(v);
}

static std::optional<std::string> pyobj_to_optional_string(PyObject* obj) {
    if (obj == Py_None) return std::nullopt;
    const char* s = PyUnicode_AsUTF8(obj);
    if (!s) return std::nullopt;
    return std::string(s);
}


// ============================================================
// Outcome type
// ============================================================

typedef struct {
    PyObject_HEAD
    Termination termination;
    // winner is only meaningful when termination == CHECKMATE (or VARIANT_WIN/LOSS)
    // We store it as an int; -1 means "no winner" (draw).
    int winner;  // 0=black, 1=white, -1=none
} PyOutcome;

static PyTypeObject PyOutcomeType = { PyVarObject_HEAD_INIT(nullptr, 0) };

static PyObject* Outcome_str(PyOutcome* self) {
    std::optional<Color> winner =
        (self->winner >= 0) ? std::optional<Color>((Color)(self->winner)) : std::nullopt;
    Outcome o{ self->termination, winner.value_or(Color::white) };
    // For draws without a winner, to_string(outcome) should not display the winner field
    std::string s = "Outcome(termination=" + to_string(self->termination) +
                    ", winner=" + (self->winner >= 0 ? to_string((Color)(self->winner)) : "None") + ")";
    return PyUnicode_FromString(s.c_str());
}

static PyObject* Outcome_get_termination(PyOutcome* self, void*) {
    return PyLong_FromLong((int)(self->termination));
}
static PyObject* Outcome_get_winner(PyOutcome* self, void*) {
    if (self->winner < 0) Py_RETURN_NONE;
    return PyLong_FromLong(self->winner);
}

static PyGetSetDef Outcome_getset[] = {
    {"termination", (getter)Outcome_get_termination, nullptr, "Termination enum int", nullptr},
    {"winner",      (getter)Outcome_get_winner,      nullptr, "Color int or None",    nullptr},
    {nullptr}
};

static PyObject* outcome_to_py(const std::optional<Outcome>& opt) {
    if (!opt) Py_RETURN_NONE;
    PyOutcome* obj = PyObject_New(PyOutcome, &PyOutcomeType);
    if (!obj) return nullptr;
    obj->termination = opt->termination;
    // Determine winner: CHECKMATE => the side that delivered it (opponent of loser)
    // VARIANT_WIN / VARIANT_LOSS encode winner in the color field of Outcome.
    // For draws there is no meaningful winner; we use -1.
    switch (opt->termination) {
        case Termination::CHECKMATE:
        case Termination::VARIANT_WIN:
        case Termination::VARIANT_LOSS:
            obj->winner = (int)(opt->winner);
            break;
        default:
            obj->winner = -1;
            break;
    }
    return (PyObject*)obj;
}

// ============================================================
// Piece type
// ============================================================

typedef struct {
    PyObject_HEAD
    Piece piece;
} PyPiece;

static PyObject* Piece_str(PyPiece* self) {
    return PyUnicode_FromString(
        (to_string(self->piece.type) + " (" + to_string(self->piece.color) + ")").c_str());
}

static int Piece_init(PyPiece* self, PyObject* args, PyObject*) {
    int type, color;
    if (!PyArg_ParseTuple(args, "ii", &type, &color)) return -1;
    self->piece.type  = (PieceType)(type);
    self->piece.color = (Color)(color);
    return 0;
}

static PyObject* Piece_get_type(PyPiece* self, void*) {
    return PyLong_FromLong((int)(self->piece.type));
}
static PyObject* Piece_get_color(PyPiece* self, void*) {
    return PyLong_FromLong((int)(self->piece.color));
}

static PyGetSetDef Piece_getset[] = {
    {"type",       (getter)Piece_get_type,  nullptr, "PieceType int", nullptr},
    {"piece_type", (getter)Piece_get_type,  nullptr, "PieceType int (alias for type)", nullptr},
    {"color",      (getter)Piece_get_color, nullptr, "Color int",     nullptr},
    {nullptr}
};

static PyTypeObject PyPieceType = { PyVarObject_HEAD_INIT(nullptr, 0) };

// ============================================================
// Move type
// ============================================================

typedef struct {
    PyObject_HEAD
    Move move;
} PyMove;

static PyTypeObject PyMoveType = { PyVarObject_HEAD_INIT(nullptr, 0) };


// ============================================================
// Helper: build a Python Move object from a C++ Move
// ============================================================

static PyObject* move_to_py(const Move& m) {
    PyMove* obj = PyObject_New(PyMove, &PyMoveType);
    if (!obj) return nullptr;
    if (m.promotion)
        obj->move = Move(m.from, m.to, *m.promotion);
    else
        obj->move = Move(m.from, m.to);
    if (m.drop) obj->move.drop = m.drop;
    return (PyObject*)obj;
}


static PyObject* Move_from_uci(PyObject* /*module*/, PyObject* args) {
    const char* uci_str;
    if (!PyArg_ParseTuple(args, "s", &uci_str)) return nullptr;
    try {
        Move m = Move::from_uci(uci_str);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}
static PyObject* Move_str(PyMove* self) {
    return PyUnicode_FromString(to_string(self->move).c_str());
}

static int Move_init(PyMove* self, PyObject* args, PyObject* kwds) {
    int from_sq, to_sq;
    PyObject* promo  = Py_None;
    static char* kw[] = {(char *)("from_square"), (char *)("to_square"),
                         (char *)("promotion"), nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii|O", kw,
                                     &from_sq, &to_sq, &promo))
        return -1;

    Square from = (Square)(from_sq);
    Square to   = (Square)(to_sq);

    if (promo != Py_None) {
        long pv = PyLong_AsLong(promo);
        if (pv == -1 && PyErr_Occurred()) return -1;
        self->move = Move(from, to, (PieceType)(pv));
    } else {
        self->move = Move(from, to);
    }
    return 0;
}



static PyObject* Move_uci(PyMove* self, PyObject*) {
    return PyUnicode_FromString(self->move.uci().c_str());
}

static PyMethodDef Move_methods[] = {
    {"from_uci", (PyCFunction)Move_from_uci, METH_VARARGS | METH_STATIC,
     "Create a Move object from a UCI string. Usage: Move.from_uci('e2e4')"},
    {"uci", (PyCFunction)Move_uci, METH_NOARGS, "Return the UCI string of the move."},
    {nullptr}
};

static PyObject* Move_get_from(PyMove* self, void*) {
    return PyLong_FromLong((int)(self->move.from));
}
static PyObject* Move_get_to(PyMove* self, void*) {
    return PyLong_FromLong((int)(self->move.to));
}
static PyObject* Move_get_promotion(PyMove* self, void*) {
    if (!self->move.promotion) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*self->move.promotion));
}
static PyObject* Move_get_drop(PyMove* self, void*) {
    if (!self->move.drop) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*self->move.drop));
}

static PyGetSetDef Move_getset[] = {
    {"from_square", (getter)Move_get_from,      nullptr, "from square",  nullptr},
    {"to_square",   (getter)Move_get_to,        nullptr, "to square",    nullptr},
    {"promotion",   (getter)Move_get_promotion, nullptr, "promotion",    nullptr},
    {"drop",        (getter)Move_get_drop,      nullptr, "drop",         nullptr},
    {nullptr}
};

// ============================================================
// BaseBoard type
// ============================================================

typedef struct {
    PyObject_HEAD
    BaseBoard* board;
} PyBaseBoard;

static int BaseBoard_init(PyBaseBoard* self, PyObject* args, PyObject*) {
    PyObject* fen_obj = Py_None;
    if (!PyArg_ParseTuple(args, "|O", &fen_obj)) return -1;
    auto fen = pyobj_to_optional_string(fen_obj);
    try {
        self->board = new BaseBoard(fen);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    return 0;
}

static void BaseBoard_dealloc(PyBaseBoard* self) {
    delete self->board;
    Py_TYPE(self)->tp_free(self);
}

// --- methods ---

static PyObject* BaseBoard_reset_board(PyBaseBoard* self, PyObject*) {
    self->board->reset_board();
    Py_RETURN_NONE;
}

static PyObject* BaseBoard_clear_board(PyBaseBoard* self, PyObject*) {
    self->board->clear_board();
    Py_RETURN_NONE;
}

static PyObject* BaseBoard_pieces_mask(PyBaseBoard* self, PyObject* args) {
    int pt, col;
    if (!PyArg_ParseTuple(args, "ii", &pt, &col)) return nullptr;
    Bitboard bb = self->board->pieces_mask((PieceType)(pt),
                                           (Color)(col));
    return PyLong_FromUnsignedLongLong(bb);
}

static PyObject* BaseBoard_piece_at(PyBaseBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto p = self->board->piece_at((Square)(sq));
    if (!p) Py_RETURN_NONE;
    PyPiece* obj = PyObject_New(PyPiece, &PyPieceType);
    if (!obj) return nullptr;
    obj->piece = *p;
    return (PyObject*)obj;
}

static PyObject* BaseBoard_piece_type_at(PyBaseBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto pt = self->board->piece_type_at((Square)(sq));
    if (!pt) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*pt));
}

static PyObject* BaseBoard_color_at(PyBaseBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto c = self->board->color_at((Square)(sq));
    if (!c) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*c));
}

static PyObject* BaseBoard_king(PyBaseBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    auto sq = self->board->king((Color)(col));
    if (!sq) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*sq));
}

static PyObject* BaseBoard_attacks_mask(PyBaseBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->attacks_mask((Square)(sq)));
}

static PyObject* BaseBoard_attackers_mask(PyBaseBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->attackers_mask((Color)(col),
                                    (Square)(sq)));
}

static PyObject* BaseBoard_is_attacked_by(PyBaseBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyBool_FromLong(
        self->board->is_attacked_by((Color)(col),
                                    (Square)(sq)));
}

static PyObject* BaseBoard_pin_mask(PyBaseBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->pin_mask((Color)(col),
                              (Square)(sq)));
}

static PyObject* BaseBoard_is_pinned(PyBaseBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyBool_FromLong(
        self->board->is_pinned((Color)(col),
                               (Square)(sq)));
}

static PyObject* BaseBoard_remove_piece_at(PyBaseBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto p = self->board->remove_piece_at((Square)(sq));
    if (!p) Py_RETURN_NONE;
    PyPiece* obj = PyObject_New(PyPiece, &PyPieceType);
    if (!obj) return nullptr;
    obj->piece = *p;
    return (PyObject*)obj;
}

static PyObject* BaseBoard_set_piece_at(PyBaseBoard* self, PyObject* args) {
    int sq;
    PyObject* piece_obj;
    int promoted = 0;
    if (!PyArg_ParseTuple(args, "iO|p", &sq, &piece_obj, &promoted))
        return nullptr;
    std::optional<Piece> piece;
    if (piece_obj != Py_None) {
        if (!PyObject_TypeCheck(piece_obj, &PyPieceType)) {
            PyErr_SetString(PyExc_TypeError, "Expected Piece or None");
            return nullptr;
        }
        piece = ((PyPiece*)piece_obj)->piece;
    }
    self->board->set_piece_at((Square)(sq), piece, promoted);
    Py_RETURN_NONE;
}

static PyObject* BaseBoard_set_board_fen(PyBaseBoard* self, PyObject* args) {
    const char* fen;
    if (!PyArg_ParseTuple(args, "s", &fen)) return nullptr;
    try { self->board->set_board_fen(fen); }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
    Py_RETURN_NONE;
}

// Single definition — must appear before BaseBoard_mirror which references it.
static PyTypeObject PyBaseBoardType = { PyVarObject_HEAD_INIT(nullptr, 0) };

static PyObject* BaseBoard_mirror(PyBaseBoard* self, PyObject*) {
    PyBaseBoard* obj = PyObject_New(PyBaseBoard, &PyBaseBoardType);
    if (!obj) return nullptr;
    obj->board = new BaseBoard(self->board->mirror());
    return (PyObject*)obj;
}

static PyMethodDef BaseBoard_methods[] = {
    {"reset_board",      (PyCFunction)BaseBoard_reset_board,    METH_NOARGS,  "Reset board to starting position"},
    {"clear_board",      (PyCFunction)BaseBoard_clear_board,    METH_NOARGS,  "Clear all pieces"},
    {"pieces_mask",      (PyCFunction)BaseBoard_pieces_mask,    METH_VARARGS, "pieces_mask(piece_type, color) -> int (Bitboard)"},
    {"piece_at",         (PyCFunction)BaseBoard_piece_at,       METH_VARARGS, "piece_at(square) -> Piece | None"},
    {"piece_type_at",    (PyCFunction)BaseBoard_piece_type_at,  METH_VARARGS, "piece_type_at(square) -> int | None"},
    {"color_at",         (PyCFunction)BaseBoard_color_at,       METH_VARARGS, "color_at(square) -> int | None"},
    {"king",             (PyCFunction)BaseBoard_king,           METH_VARARGS, "king(color) -> int | None"},
    {"attacks_mask",     (PyCFunction)BaseBoard_attacks_mask,   METH_VARARGS, "attacks_mask(square) -> int (Bitboard)"},
    {"attackers_mask",   (PyCFunction)BaseBoard_attackers_mask, METH_VARARGS, "attackers_mask(color, square) -> int (Bitboard)"},
    {"is_attacked_by",   (PyCFunction)BaseBoard_is_attacked_by, METH_VARARGS, "is_attacked_by(color, square) -> bool"},
    {"pin_mask",         (PyCFunction)BaseBoard_pin_mask,       METH_VARARGS, "pin_mask(color, square) -> int (Bitboard)"},
    {"is_pinned",        (PyCFunction)BaseBoard_is_pinned,      METH_VARARGS, "is_pinned(color, square) -> bool"},
    {"remove_piece_at",  (PyCFunction)BaseBoard_remove_piece_at,METH_VARARGS, "remove_piece_at(square) -> Piece | None"},
    {"set_piece_at",     (PyCFunction)BaseBoard_set_piece_at,   METH_VARARGS, "set_piece_at(square, piece, promoted=False)"},
    {"set_board_fen",    (PyCFunction)BaseBoard_set_board_fen,  METH_VARARGS, "set_board_fen(fen)"},
    {"mirror",           (PyCFunction)BaseBoard_mirror,         METH_NOARGS,  "mirror() -> BaseBoard"},
    {nullptr}
};

// Expose raw bitboard fields as Python integers
static PyObject* BaseBoard_get_pawns(PyBaseBoard* s, void*)   { return PyLong_FromUnsignedLongLong(s->board->pawns); }
static PyObject* BaseBoard_get_knights(PyBaseBoard* s, void*) { return PyLong_FromUnsignedLongLong(s->board->knights); }
static PyObject* BaseBoard_get_bishops(PyBaseBoard* s, void*) { return PyLong_FromUnsignedLongLong(s->board->bishops); }
static PyObject* BaseBoard_get_rooks(PyBaseBoard* s, void*)   { return PyLong_FromUnsignedLongLong(s->board->rooks); }
static PyObject* BaseBoard_get_queens(PyBaseBoard* s, void*)  { return PyLong_FromUnsignedLongLong(s->board->queens); }
static PyObject* BaseBoard_get_kings(PyBaseBoard* s, void*)   { return PyLong_FromUnsignedLongLong(s->board->kings); }
static PyObject* BaseBoard_get_occupied(PyBaseBoard* s, void*){ return PyLong_FromUnsignedLongLong(s->board->occupied); }

static PyGetSetDef BaseBoard_getset[] = {
    {"pawns",    (getter)BaseBoard_get_pawns,    nullptr, "pawn bitboard",    nullptr},
    {"knights",  (getter)BaseBoard_get_knights,  nullptr, "knight bitboard",  nullptr},
    {"bishops",  (getter)BaseBoard_get_bishops,  nullptr, "bishop bitboard",  nullptr},
    {"rooks",    (getter)BaseBoard_get_rooks,    nullptr, "rook bitboard",    nullptr},
    {"queens",   (getter)BaseBoard_get_queens,   nullptr, "queen bitboard",   nullptr},
    {"kings",    (getter)BaseBoard_get_kings,    nullptr, "king bitboard",    nullptr},
    {"occupied", (getter)BaseBoard_get_occupied, nullptr, "occupied bitboard",nullptr},
    {nullptr}
};

// ============================================================
// Board type  (inherits PyBaseBoard layout by being a superset)
// ============================================================

typedef struct {
    PyObject_HEAD
    Board* board;  // Board IS-A BaseBoard, cast as needed
} PyBoard;

static int Board_init(PyBoard* self, PyObject* args, PyObject*) {
    PyObject* fen_obj = Py_None;
    if (!PyArg_ParseTuple(args, "|O", &fen_obj)) return -1;
    auto fen = pyobj_to_optional_string(fen_obj);
    try {
        self->board = fen ? new Board(*fen) : new Board();
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    return 0;
}

static void Board_dealloc(PyBoard* self) {
    delete self->board;
    Py_TYPE(self)->tp_free(self);
}

// ---- helper: build list of Move objects from vector<Move> ----
static PyObject* move_vec_to_list(const vector<Move>& moves) {
    PyObject* lst = PyList_New(moves.size());
    if (!lst) return nullptr;
    for (Py_ssize_t i = 0; i < (Py_ssize_t)(moves.size()); i++) {
        PyObject* m = move_to_py(moves[i]);
        if (!m) { Py_DECREF(lst); return nullptr; }
        PyList_SET_ITEM(lst, i, m);
    }
    return lst;
}

// ---- get a Move* from a PyObject (must be PyMove) ----
static std::optional<Move> py_to_move(PyObject* obj) {
    if (!PyObject_TypeCheck(obj, &PyMoveType)) {
        PyErr_SetString(PyExc_TypeError, "Expected a Move object");
        return std::nullopt;  // Return a default Move; caller should check for error
    }
    return ((PyMove*)obj)->move;
}

// ---- Board methods ----

static PyObject* Board_reset(PyBoard* self, PyObject*) {
    self->board->reset(); Py_RETURN_NONE;
}
static PyObject* Board_clear(PyBoard* self, PyObject*) {
    self->board->clear(); Py_RETURN_NONE;
}
static PyObject* Board_clear_stack(PyBoard* self, PyObject*) {
    self->board->clear_stack(); Py_RETURN_NONE;
}
static PyObject* Board_ply(PyBoard* self, PyObject*) {
    return PyLong_FromLong(self->board->ply());
}

static PyObject* Board_generate_legal_moves(PyBoard* self, PyObject* args) {
    unsigned long long from_mask = BB_ALL, to_mask = BB_ALL;
    if (!PyArg_ParseTuple(args, "|KK", &from_mask, &to_mask)) return nullptr;
    return move_vec_to_list(self->board->generate_legal_moves(from_mask, to_mask));
}

static PyObject* Board_generate_pseudo_legal_moves(PyBoard* self, PyObject* args) {
    unsigned long long from_mask = BB_ALL, to_mask = BB_ALL;
    if (!PyArg_ParseTuple(args, "|KK", &from_mask, &to_mask)) return nullptr;
    return move_vec_to_list(self->board->generate_pseudo_legal_moves(from_mask, to_mask));
}

static PyObject* Board_generate_legal_captures(PyBoard* self, PyObject* args) {
    unsigned long long from_mask = BB_ALL, to_mask = BB_ALL;
    if (!PyArg_ParseTuple(args, "|KK", &from_mask, &to_mask)) return nullptr;
    return move_vec_to_list(self->board->generate_legal_captures(from_mask, to_mask));
}

static PyObject* Board_generate_castling_moves(PyBoard* self, PyObject* args) {
    unsigned long long from_mask = BB_ALL, to_mask = BB_ALL;
    if (!PyArg_ParseTuple(args, "|KK", &from_mask, &to_mask)) return nullptr;
    return move_vec_to_list(self->board->generate_castling_moves(from_mask, to_mask));
}

static PyObject* Board_is_check(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_check());
}
static PyObject* Board_is_checkmate(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_checkmate());
}
static PyObject* Board_is_stalemate(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_stalemate());
}
static PyObject* Board_is_insufficient_material(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_insufficient_material());
}
static PyObject* Board_is_game_over(PyBoard* self, PyObject* args) {
    int claim = 0;
    if (!PyArg_ParseTuple(args, "|p", &claim)) return nullptr;
    return PyBool_FromLong(self->board->is_game_over(claim));
}
static PyObject* Board_result(PyBoard* self, PyObject* args) {
    int claim = 0;
    if (!PyArg_ParseTuple(args, "|p", &claim)) return nullptr;
    std::string r = self->board->result(claim);
    return PyUnicode_FromString(r.c_str());
}


static PyObject* Board_outcome(PyBoard* self, PyObject* args) {
    int claim = 0;
    if (!PyArg_ParseTuple(args, "|p", &claim)) return nullptr;
    auto opt = self->board->outcome(claim);
    return outcome_to_py(opt);
}

static PyObject* Board_checkers_mask(PyBoard* self, PyObject*) {
    return PyLong_FromUnsignedLongLong(self->board->checkers_mask());
}

static PyObject* Board_gives_check(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->gives_check(*m));
}

static PyObject* Board_is_legal(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_legal(*m));
}

static PyObject* Board_is_pseudo_legal(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_pseudo_legal(*m));
}

static PyObject* Board_is_capture(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_capture(*m));
}

static PyObject* Board_is_en_passant(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_en_passant(*m));
}

static PyObject* Board_is_castling(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_castling(*m));
}

static PyObject* Board_is_kingside_castling(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_kingside_castling(*m));
}

static PyObject* Board_is_queenside_castling(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    return PyBool_FromLong(self->board->is_queenside_castling(*m));
}

static PyObject* Board_push(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    try { self->board->push(*m); }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what()); return nullptr;
    }
    Py_RETURN_NONE;
}

static PyObject* Board_pop(PyBoard* self, PyObject*) {
    try {
        Move m = self->board->pop();
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what()); return nullptr;
    }
}

static PyObject* Board_peek(PyBoard* self, PyObject*) {
    try {
        Move m = self->board->peek();
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what()); return nullptr;
    }
}

static PyObject* Board_san(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    std::string s = self->board->san(*m);
    return PyUnicode_FromString(s.c_str());
}

static PyObject* Board_lan(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    std::string s = self->board->lan(*m);
    return PyUnicode_FromString(s.c_str());
}

static PyObject* Board_parse_san(PyBoard* self, PyObject* args) {
    const char* san;
    if (!PyArg_ParseTuple(args, "s", &san)) return nullptr;
    try {
        Move m = self->board->parse_san(san);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
}

static PyObject* Board_push_san(PyBoard* self, PyObject* args) {
    const char* san;
    if (!PyArg_ParseTuple(args, "s", &san)) return nullptr;
    try {
        Move m = self->board->push_san(san);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
}

static PyObject* Board_uci(PyBoard* self, PyObject* args) {
    PyObject* move_obj;
    if (!PyArg_ParseTuple(args, "O", &move_obj)) return nullptr;
    std::optional<Move> m = py_to_move(move_obj);
    if (!m) return nullptr;
    std::string s = self->board->uci(*m);
    return PyUnicode_FromString(s.c_str());
}

static PyObject* Board_parse_uci(PyBoard* self, PyObject* args) {
    const char* uci;
    if (!PyArg_ParseTuple(args, "s", &uci)) return nullptr;
    try {
        Move m = self->board->parse_uci(uci);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
}

static PyObject* Board_push_uci(PyBoard* self, PyObject* args) {
    const char* uci;
    if (!PyArg_ParseTuple(args, "s", &uci)) return nullptr;
    try {
        Move m = self->board->push_uci(uci);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
}

static PyObject* Board_fen(PyBoard* self, PyObject* args, PyObject* kwds) {
    int shredder = 0;
    const char* ep = "legal";
    static char* kw[] = {(char *)("shredder"), (char *)("en_passant"), nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ps", kw, &shredder, &ep))
        return nullptr;
    std::string f = self->board->fen(shredder, ep);
    return PyUnicode_FromString(f.c_str());
}

static PyObject* Board_set_fen(PyBoard* self, PyObject* args) {
    const char* fen;
    if (!PyArg_ParseTuple(args, "s", &fen)) return nullptr;
    try { self->board->set_fen(fen); }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
    Py_RETURN_NONE;
}

static PyObject* Board_is_valid(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_valid());
}

static PyObject* Board_is_repetition(PyBoard* self, PyObject* args) {
    int count = 3;
    if (!PyArg_ParseTuple(args, "|i", &count)) return nullptr;
    return PyBool_FromLong(self->board->is_repetition(count));
}

static PyObject* Board_can_claim_draw(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->can_claim_draw());
}

static PyObject* Board_has_castling_rights(PyBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    return PyBool_FromLong(self->board->has_castling_rights((Color)(col)));
}

static PyObject* Board_has_kingside_castling_rights(PyBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    return PyBool_FromLong(self->board->has_kingside_castling_rights((Color)(col)));
}

static PyObject* Board_has_queenside_castling_rights(PyBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    return PyBool_FromLong(self->board->has_queenside_castling_rights((Color)(col)));
}

static PyObject* Board_find_move(PyBoard* self, PyObject* args) {
    int from_sq, to_sq;
    PyObject* promo = Py_None;
    if (!PyArg_ParseTuple(args, "ii|O", &from_sq, &to_sq, &promo)) return nullptr;
    auto promotion = pyobj_to_optional_piece_type(promo);
    if (promo != Py_None && !promotion) return nullptr;  // error already set
    try {
        Move m = self->board->find_move((Square)(from_sq),
                                        (Square)(to_sq), promotion);
        return move_to_py(m);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what()); return nullptr;
    }
}

static PyObject* Board_variation_san(PyBoard* self, PyObject* args) {
    PyObject* lst;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &lst)) return nullptr;
    vector<Move> moves;
    Py_ssize_t n = PyList_GET_SIZE(lst);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject* item = PyList_GET_ITEM(lst, i);
        std::optional<Move> m = py_to_move(item);
        if (!m) return nullptr;
        moves.push_back(*m);
    }
    std::string s = self->board->variation_san(moves);
    return PyUnicode_FromString(s.c_str());
}

// ---- Board getset: expose common fields ----

static PyObject* Board_get_turn(PyBoard* s, void*) {
    return PyLong_FromLong((int)(s->board->turn));
}
static PyObject* Board_get_fullmove(PyBoard* s, void*) {
    return PyLong_FromLong(s->board->fullmove_number);
}
static PyObject* Board_get_halfmove(PyBoard* s, void*) {
    return PyLong_FromLong(s->board->halfmove_clock);
}
static PyObject* Board_get_ep_square(PyBoard* s, void*) {
    if (!s->board->ep_square) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*s->board->ep_square));
}
static PyObject* Board_get_castling_rights(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->castling_rights);
}
static PyObject* Board_get_pawns(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->pawns);
}
static PyObject* Board_get_knights(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->knights);
}
static PyObject* Board_get_bishops(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->bishops);
}
static PyObject* Board_get_rooks(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->rooks);
}
static PyObject* Board_get_queens(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->queens);
}
static PyObject* Board_get_kings(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->kings);
}
static PyObject* Board_get_occupied(PyBoard* s, void*) {
    return PyLong_FromUnsignedLongLong(s->board->occupied);
}

// legal_moves / pseudo_legal_moves as properties (python-chess compatibility)
static PyObject* Board_get_legal_moves(PyBoard* s, void*) {
    PyObject* lst = PyList_New(0);
    if (!lst) return nullptr;
    try {
        for (Move mv : s->board->generate_legal_moves()) {
            PyMove* obj = PyObject_New(PyMove, &PyMoveType);
            if (!obj) { Py_DECREF(lst); return nullptr; }
            obj->move = Move(mv);
            if (PyList_Append(lst, (PyObject*)obj) < 0) {
                Py_DECREF(obj); Py_DECREF(lst); return nullptr;
            }
            Py_DECREF(obj);
        }
    } catch (const std::exception& e) {
        Py_DECREF(lst);
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
    return lst;
}
static PyObject* Board_get_pseudo_legal_moves(PyBoard* s, void*) {
    PyObject* lst = PyList_New(0);
    if (!lst) return nullptr;
    try {
        for (Move mv : s->board->generate_pseudo_legal_moves()) {
            PyMove* obj = PyObject_New(PyMove, &PyMoveType);
            if (!obj) { Py_DECREF(lst); return nullptr; }
            obj->move = Move(mv);
            if (PyList_Append(lst, (PyObject*)obj) < 0) {
                Py_DECREF(obj); Py_DECREF(lst); return nullptr;
            }
            Py_DECREF(obj);
        }
    } catch (const std::exception& e) {
        Py_DECREF(lst);
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
    return lst;
}

static PyGetSetDef Board_getset[] = {
    {"legal_moves",        (getter)Board_get_legal_moves,        nullptr, "list of legal moves",        nullptr},
    {"pseudo_legal_moves", (getter)Board_get_pseudo_legal_moves, nullptr, "list of pseudo-legal moves", nullptr},
    {"turn",            (getter)Board_get_turn,            nullptr, "Side to move (0=black, 1=white)", nullptr},
    {"fullmove_number", (getter)Board_get_fullmove,        nullptr, "Fullmove number",                 nullptr},
    {"halfmove_clock",  (getter)Board_get_halfmove,        nullptr, "Halfmove clock",                  nullptr},
    {"ep_square",       (getter)Board_get_ep_square,       nullptr, "En passant square or None",       nullptr},
    {"castling_rights", (getter)Board_get_castling_rights, nullptr, "Castling rights bitboard",        nullptr},
    {"pawns",    (getter)Board_get_pawns,    nullptr, "pawn bitboard",    nullptr},
    {"knights",  (getter)Board_get_knights,  nullptr, "knight bitboard",  nullptr},
    {"bishops",  (getter)Board_get_bishops,  nullptr, "bishop bitboard",  nullptr},
    {"rooks",    (getter)Board_get_rooks,    nullptr, "rook bitboard",    nullptr},
    {"queens",   (getter)Board_get_queens,   nullptr, "queen bitboard",   nullptr},
    {"kings",    (getter)Board_get_kings,    nullptr, "king bitboard",    nullptr},
    {"occupied", (getter)Board_get_occupied, nullptr, "occupied bitboard",nullptr},
    {nullptr}
};


// ============================================================
// Board wrappers for BaseBoard methods
// (tp_base inheritance is kept for Python MRO, but these
//  explicit overrides ensure self->board is always Board*,
//  avoiding the PyBaseBoard* cast bug.)
// ============================================================

static PyObject* Board_reset_board(PyBoard* self, PyObject*) {
    self->board->reset_board(); Py_RETURN_NONE;
}
static PyObject* Board_clear_board(PyBoard* self, PyObject*) {
    self->board->clear_board(); Py_RETURN_NONE;
}
static PyObject* Board_pieces_mask(PyBoard* self, PyObject* args) {
    int pt, col;
    if (!PyArg_ParseTuple(args, "ii", &pt, &col)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->pieces_mask((PieceType)(pt), (Color)(col)));
}
static PyObject* Board_piece_at(PyBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto p = self->board->piece_at((Square)(sq));
    if (!p) Py_RETURN_NONE;
    PyPiece* obj = PyObject_New(PyPiece, &PyPieceType);
    if (!obj) return nullptr;
    obj->piece = *p;
    return (PyObject*)obj;
}
static PyObject* Board_piece_type_at(PyBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto pt = self->board->piece_type_at((Square)(sq));
    if (!pt) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*pt));
}
static PyObject* Board_color_at(PyBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto c = self->board->color_at((Square)(sq));
    if (!c) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*c));
}
static PyObject* Board_king(PyBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    auto sq = self->board->king((Color)(col));
    if (!sq) Py_RETURN_NONE;
    return PyLong_FromLong((int)(*sq));
}
static PyObject* Board_attacks_mask(PyBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->attacks_mask((Square)(sq)));
}
static PyObject* Board_attackers_mask(PyBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->attackers_mask((Color)(col), (Square)(sq)));
}
static PyObject* Board_is_attacked_by(PyBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyBool_FromLong(
        self->board->is_attacked_by((Color)(col), (Square)(sq)));
}
static PyObject* Board_pin_mask(PyBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyLong_FromUnsignedLongLong(
        self->board->pin_mask((Color)(col), (Square)(sq)));
}
static PyObject* Board_is_pinned(PyBoard* self, PyObject* args) {
    int col, sq;
    if (!PyArg_ParseTuple(args, "ii", &col, &sq)) return nullptr;
    return PyBool_FromLong(
        self->board->is_pinned((Color)(col), (Square)(sq)));
}
static PyObject* Board_remove_piece_at(PyBoard* self, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    auto p = self->board->remove_piece_at((Square)(sq));
    if (!p) Py_RETURN_NONE;
    PyPiece* obj = PyObject_New(PyPiece, &PyPieceType);
    if (!obj) return nullptr;
    obj->piece = *p;
    return (PyObject*)obj;
}
static PyObject* Board_set_piece_at(PyBoard* self, PyObject* args) {
    int sq;
    PyObject* piece_obj;
    int promoted = 0;
    if (!PyArg_ParseTuple(args, "iO|p", &sq, &piece_obj, &promoted)) return nullptr;
    std::optional<Piece> piece;
    if (piece_obj != Py_None) {
        if (!PyObject_TypeCheck(piece_obj, &PyPieceType)) {
            PyErr_SetString(PyExc_TypeError, "Expected Piece or None"); return nullptr;
        }
        piece = ((PyPiece*)piece_obj)->piece;
    }
    self->board->set_piece_at((Square)(sq), piece, promoted);
    Py_RETURN_NONE;
}
static PyObject* Board_set_board_fen(PyBoard* self, PyObject* args) {
    const char* fen;
    if (!PyArg_ParseTuple(args, "s", &fen)) return nullptr;
    try { self->board->set_board_fen(fen); }
    catch (const std::exception& e) { PyErr_SetString(PyExc_ValueError, e.what()); return nullptr; }
    Py_RETURN_NONE;
}
static PyTypeObject PyBoardType = { PyVarObject_HEAD_INIT(nullptr, 0) };

static PyObject* Board_mirror_board(PyBoard* self, PyObject*) {
    // Returns a new Board (not BaseBoard) mirrored
    PyBoard* obj = PyObject_New(PyBoard, &PyBoardType);
    if (!obj) return nullptr;
    obj->board = new Board(self->board->mirror());
    return (PyObject*)obj;
}


// ── Missing python-chess Board methods ──────────────────────────────────
static PyObject* Board_copy(PyBoard* self, PyObject*) {
    PyBoard* obj = PyObject_New(PyBoard, &PyBoardType);
    if (!obj) return nullptr;
    obj->board = new Board(*self->board);
    return (PyObject*)obj;
}
static PyObject* Board_is_seventyfive_moves(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_seventyfive_moves());
}
static PyObject* Board_is_fivefold_repetition(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_fivefold_repetition());
}
static PyObject* Board_is_fifty_moves(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_fifty_moves());
}
static PyObject* Board_can_claim_fifty_moves(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->can_claim_fifty_moves());
}
static PyObject* Board_can_claim_threefold_repetition(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->can_claim_threefold_repetition());
}
static PyObject* Board_is_variant_end(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_variant_end());
}
static PyObject* Board_is_variant_loss(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_variant_loss());
}
static PyObject* Board_is_variant_win(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_variant_win());
}
static PyObject* Board_is_variant_draw(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->is_variant_draw());
}
static PyObject* Board_has_insufficient_material(PyBoard* self, PyObject* args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) return nullptr;
    return PyBool_FromLong(self->board->has_insufficient_material((Color)(col)));
}
static PyObject* Board_was_into_check(PyBoard* self, PyObject*) {
    return PyBool_FromLong(self->board->was_into_check());
}

static PyMethodDef Board_methods[] = {
    {"copy",                            (PyCFunction) Board_copy,                            METH_NOARGS,   "copy() -> Board"},
    {"is_seventyfive_moves",            (PyCFunction) Board_is_seventyfive_moves,            METH_NOARGS,   "is_seventyfive_moves() -> bool"},
    {"is_fivefold_repetition",          (PyCFunction) Board_is_fivefold_repetition,          METH_NOARGS,   "is_fivefold_repetition() -> bool"},
    {"is_fifty_moves",                  (PyCFunction) Board_is_fifty_moves,                  METH_NOARGS,   "is_fifty_moves() -> bool"},
    {"can_claim_fifty_moves",           (PyCFunction) Board_can_claim_fifty_moves,           METH_NOARGS,   "can_claim_fifty_moves() -> bool"},
    {"can_claim_threefold_repetition",  (PyCFunction) Board_can_claim_threefold_repetition,  METH_NOARGS,   "can_claim_threefold_repetition() -> bool"},
    {"is_variant_end",                  (PyCFunction) Board_is_variant_end,                  METH_NOARGS,   "is_variant_end() -> bool"},
    {"is_variant_loss",                 (PyCFunction) Board_is_variant_loss,                 METH_NOARGS,   "is_variant_loss() -> bool"},
    {"is_variant_win",                  (PyCFunction) Board_is_variant_win,                  METH_NOARGS,   "is_variant_win() -> bool"},
    {"is_variant_draw",                 (PyCFunction) Board_is_variant_draw,                 METH_NOARGS,   "is_variant_draw() -> bool"},
    {"has_insufficient_material",       (PyCFunction) Board_has_insufficient_material,       METH_VARARGS,  "has_insufficient_material(color) -> bool"},
    {"was_into_check",                  (PyCFunction) Board_was_into_check,                  METH_NOARGS,   "was_into_check() -> bool"},
    // BaseBoard methods duplicated on Board to avoid PyBaseBoard* cast bug
    {"reset_board",                     (PyCFunction) Board_reset_board,                     METH_NOARGS,   "reset_board()"},
    {"clear_board",                     (PyCFunction) Board_clear_board,                     METH_NOARGS,   "clear_board()"},
    {"pieces_mask",                     (PyCFunction) Board_pieces_mask,                     METH_VARARGS,  "pieces_mask(piece_type, color) -> int"},
    {"piece_at",                        (PyCFunction) Board_piece_at,                        METH_VARARGS,  "piece_at(square) -> Piece | None"},
    {"piece_type_at",                   (PyCFunction) Board_piece_type_at,                   METH_VARARGS,  "piece_type_at(square) -> int | None"},
    {"color_at",                        (PyCFunction) Board_color_at,                        METH_VARARGS,  "color_at(square) -> int | None"},
    {"king",                            (PyCFunction) Board_king,                            METH_VARARGS,  "king(color) -> int | None"},
    {"attacks_mask",                    (PyCFunction) Board_attacks_mask,                    METH_VARARGS,  "attacks_mask(square) -> int"},
    {"attackers_mask",                  (PyCFunction) Board_attackers_mask,                  METH_VARARGS,  "attackers_mask(color, square) -> int"},
    {"is_attacked_by",                  (PyCFunction) Board_is_attacked_by,                  METH_VARARGS,  "is_attacked_by(color, square) -> bool"},
    {"pin_mask",                        (PyCFunction) Board_pin_mask,                        METH_VARARGS,  "pin_mask(color, square) -> int"},
    {"is_pinned",                       (PyCFunction) Board_is_pinned,                       METH_VARARGS,  "is_pinned(color, square) -> bool"},
    {"remove_piece_at",                 (PyCFunction) Board_remove_piece_at,                 METH_VARARGS,  "remove_piece_at(square) -> Piece | None"},
    {"set_piece_at",                    (PyCFunction) Board_set_piece_at,                    METH_VARARGS,  "set_piece_at(square, piece, promoted=False)"},
    {"set_board_fen",                   (PyCFunction) Board_set_board_fen,                   METH_VARARGS,  "set_board_fen(fen)"},
    {"mirror",                          (PyCFunction) Board_mirror_board,                    METH_NOARGS,   "mirror() -> Board"},
    {"reset",                           (PyCFunction) Board_reset,                           METH_NOARGS,   "Reset to starting position"},
    {"clear",                           (PyCFunction) Board_clear,                           METH_NOARGS,   "Clear board and stack"},
    {"clear_stack",                     (PyCFunction) Board_clear_stack,                     METH_NOARGS,   "Clear move stack"},
    {"ply",                             (PyCFunction) Board_ply,                             METH_NOARGS,   "Number of half-moves played"},
    {"generate_legal_moves",            (PyCFunction) Board_generate_legal_moves,            METH_VARARGS,  "generate_legal_moves(from_mask=ALL, to_mask=ALL) -> list[Move]"},
    {"generate_pseudo_legal_moves",     (PyCFunction) Board_generate_pseudo_legal_moves,     METH_VARARGS,  "generate_pseudo_legal_moves(from_mask=ALL, to_mask=ALL) -> list[Move]"},
    {"generate_legal_captures",         (PyCFunction) Board_generate_legal_captures,         METH_VARARGS,  "generate_legal_captures(from_mask=ALL, to_mask=ALL) -> list[Move]"},
    {"generate_castling_moves",         (PyCFunction) Board_generate_castling_moves,         METH_VARARGS,  "generate_castling_moves(from_mask=ALL, to_mask=ALL) -> list[Move]"},
    {"is_check",                        (PyCFunction) Board_is_check,                        METH_NOARGS,   "is_check() -> bool"},
    {"is_checkmate",                    (PyCFunction) Board_is_checkmate,                    METH_NOARGS,   "is_checkmate() -> bool"},
    {"is_stalemate",                    (PyCFunction) Board_is_stalemate,                    METH_NOARGS,   "is_stalemate() -> bool"},
    {"is_insufficient_material",        (PyCFunction) Board_is_insufficient_material,        METH_NOARGS,   "is_insufficient_material() -> bool"},
    {"is_game_over",                    (PyCFunction) Board_is_game_over,                    METH_VARARGS,  "is_game_over(claim_draw=False) -> bool"},
    {"result",                          (PyCFunction) Board_result,                          METH_VARARGS,  "result(claim_draw=False) -> str"},
    {"outcome",                         (PyCFunction) Board_outcome,                         METH_VARARGS,  "outcome(claim_draw=False) -> Outcome | None"},
    {"checkers_mask",                   (PyCFunction) Board_checkers_mask,                   METH_NOARGS,   "checkers_mask() -> int (Bitboard)"},
    {"gives_check",                     (PyCFunction) Board_gives_check,                     METH_VARARGS,  "gives_check(move) -> bool"},
    {"is_legal",                        (PyCFunction) Board_is_legal,                        METH_VARARGS,  "is_legal(move) -> bool"},
    {"is_pseudo_legal",                 (PyCFunction) Board_is_pseudo_legal,                 METH_VARARGS,  "is_pseudo_legal(move) -> bool"},
    {"is_capture",                      (PyCFunction) Board_is_capture,                      METH_VARARGS,  "is_capture(move) -> bool"},
    {"is_en_passant",                   (PyCFunction) Board_is_en_passant,                   METH_VARARGS,  "is_en_passant(move) -> bool"},
    {"is_castling",                     (PyCFunction) Board_is_castling,                     METH_VARARGS,  "is_castling(move) -> bool"},
    {"is_kingside_castling",            (PyCFunction) Board_is_kingside_castling,            METH_VARARGS,  "is_kingside_castling(move) -> bool"},
    {"is_queenside_castling",           (PyCFunction) Board_is_queenside_castling,           METH_VARARGS,  "is_queenside_castling(move) -> bool"},
    {"push",                            (PyCFunction) Board_push,                            METH_VARARGS,  "push(move)"},
    {"pop",                             (PyCFunction) Board_pop,                             METH_NOARGS,   "pop() -> Move"},
    {"peek",                            (PyCFunction) Board_peek,                            METH_NOARGS,   "peek() -> Move"},
    {"san",                             (PyCFunction) Board_san,                             METH_VARARGS,  "san(move) -> str"},
    {"lan",                             (PyCFunction) Board_lan,                             METH_VARARGS,  "lan(move) -> str"},
    {"parse_san",                       (PyCFunction) Board_parse_san,                       METH_VARARGS,  "parse_san(san) -> Move"},
    {"push_san",                        (PyCFunction) Board_push_san,                        METH_VARARGS,  "push_san(san) -> Move"},
    {"uci",                             (PyCFunction) Board_uci,                             METH_VARARGS,  "uci(move) -> str"},
    {"parse_uci",                       (PyCFunction) Board_parse_uci,                       METH_VARARGS,  "parse_uci(uci) -> Move"},
    {"push_uci",                        (PyCFunction) Board_push_uci,                        METH_VARARGS,  "push_uci(uci) -> Move"},
    {"fen",                             (PyCFunction) Board_fen,                             METH_VARARGS | METH_KEYWORDS, "fen(shredder=False, en_passant='legal') -> str"},
    {"set_fen",                         (PyCFunction) Board_set_fen,                         METH_VARARGS,  "set_fen(fen)"},
    {"is_valid",                        (PyCFunction) Board_is_valid,                        METH_NOARGS,   "is_valid() -> bool"},
    {"is_repetition",                   (PyCFunction) Board_is_repetition,                   METH_VARARGS,  "is_repetition(count=3) -> bool"},
    {"can_claim_draw",                  (PyCFunction) Board_can_claim_draw,                  METH_NOARGS,   "can_claim_draw() -> bool"},
    {"has_castling_rights",             (PyCFunction) Board_has_castling_rights,             METH_VARARGS,  "has_castling_rights(color) -> bool"},
    {"has_kingside_castling_rights",    (PyCFunction) Board_has_kingside_castling_rights,    METH_VARARGS,  "has_kingside_castling_rights(color) -> bool"},
    {"has_queenside_castling_rights",   (PyCFunction) Board_has_queenside_castling_rights,   METH_VARARGS,  "has_queenside_castling_rights(color) -> bool"},
    {"find_move",                       (PyCFunction) Board_find_move,                       METH_VARARGS,  "find_move(from_square, to_square, promotion=None) -> Move"},
    {"variation_san",                   (PyCFunction) Board_variation_san,                   METH_VARARGS,  "variation_san(moves: list[Move]) -> str"},
    {nullptr}
};

// ============================================================
// Module-level functions
// ============================================================

static PyObject* py_parse_square(PyObject*, PyObject* args) {
    const char* s;
    if (!PyArg_ParseTuple(args, "s", &s)) return nullptr;
    Square sq = parse_square(s);
    return PyLong_FromLong((int)(sq));
}

static PyObject* py_square_at(PyObject*, PyObject* args) {
    int file, rank;
    if (!PyArg_ParseTuple(args, "ii", &file, &rank)) return nullptr;
    return PyLong_FromLong((int)(at(file, rank)));
}

static PyObject* py_file(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong(file((Square)(sq)));
}

static PyObject* py_rank(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong(rank((Square)(sq)));
}

static PyObject* py_square_distance(PyObject*, PyObject* args) {
    int a, b;
    if (!PyArg_ParseTuple(args, "ii", &a, &b)) return nullptr;
    return PyLong_FromLong(square_distance((Square)(a), (Square)(b)));
}

static PyObject* py_square_manhattan_distance(PyObject*, PyObject* args) {
    int a, b;
    if (!PyArg_ParseTuple(args, "ii", &a, &b)) return nullptr;
    return PyLong_FromLong(square_manhattan_distance((Square)(a), (Square)(b)));
}

static PyObject* py_square_knight_distance(PyObject*, PyObject* args) {
    int a, b;
    if (!PyArg_ParseTuple(args, "ii", &a, &b)) return nullptr;
    return PyLong_FromLong(square_knight_distance((Square)(a), (Square)(b)));
}

static PyObject* py_mirror_square(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong((int)(mirror((Square)(sq))));
}

static PyObject* py_lsb(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromLong(lsb(bb));
}

static PyObject* py_msb(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromLong(msb(bb));
}

static PyObject* py_scan_forward(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    PyObject* lst = PyList_New(0);
    if (!lst) return nullptr;
    for (Square sq : scan_forward(bb)) {
        PyObject* v = PyLong_FromLong((int)(sq));
        if (!v || PyList_Append(lst, v) < 0) { Py_XDECREF(v); Py_DECREF(lst); return nullptr; }
        Py_DECREF(v);
    }
    return lst;
}

static PyObject* py_scan_reversed(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    PyObject* lst = PyList_New(0);
    if (!lst) return nullptr;
    for (Square sq : scan_reversed(bb)) {
        PyObject* v = PyLong_FromLong((int)(sq));
        if (!v || PyList_Append(lst, v) < 0) { Py_XDECREF(v); Py_DECREF(lst); return nullptr; }
        Py_DECREF(v);
    }
    return lst;
}

static PyObject* py_ray(PyObject*, PyObject* args) {
    int from, to;
    if (!PyArg_ParseTuple(args, "ii", &from, &to)) return nullptr;
    return PyLong_FromUnsignedLongLong(ray((Square)(from), (Square)(to)));
}

static PyObject* py_between(PyObject*, PyObject* args) {
    int from, to;
    if (!PyArg_ParseTuple(args, "ii", &from, &to)) return nullptr;
    return PyLong_FromUnsignedLongLong(between((Square)(from), (Square)(to)));
}

static PyObject* py_flip_vertical(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromUnsignedLongLong(flip_vertical(bb));
}

static PyObject* py_flip_horizontal(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromUnsignedLongLong(flip_horizontal(bb));
}


// ── python-chess compatible aliases and missing functions ────────────────

static PyObject* py_square_file(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong(file((Square)(sq)));
}
static PyObject* py_square_rank(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong(rank((Square)(sq)));
}
static PyObject* py_square_mirror(PyObject*, PyObject* args) {
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    return PyLong_FromLong((int)(mirror((Square)(sq))));
}
static PyObject* py_square_name(PyObject*, PyObject* args) {
    // "a1".."h8"
    static const char* FILE_NAMES = "abcdefgh";
    int sq;
    if (!PyArg_ParseTuple(args, "i", &sq)) return nullptr;
    int f = (int)(file((Square)(sq)));
    int r = (int)(rank((Square)(sq)));
    char name[3] = { FILE_NAMES[f], (char)('1' + r), 0 };
    return PyUnicode_FromString(name);
}
static PyObject* py_square_function(PyObject*, PyObject* args) {
    // square(file_index, rank_index) -> square
    int f, r;
    if (!PyArg_ParseTuple(args, "ii", &f, &r)) return nullptr;
    return PyLong_FromLong((int)(at(f, r)));
}
static const char* PIECE_SYMBOLS_ARR[] = { nullptr, "p", "n", "b", "r", "q", "k" };
static const char* PIECE_NAMES_ARR[]   = { nullptr, "pawn", "knight", "bishop", "rook", "queen", "king" };

static PyObject* py_piece_symbol(PyObject*, PyObject* args) {
    int pt;
    if (!PyArg_ParseTuple(args, "i", &pt)) return nullptr;
    if (pt < 1 || pt > 6) { PyErr_SetString(PyExc_ValueError, "invalid piece type"); return nullptr; }
    return PyUnicode_FromString(PIECE_SYMBOLS_ARR[pt]);
}
static PyObject* py_piece_name(PyObject*, PyObject* args) {
    int pt;
    if (!PyArg_ParseTuple(args, "i", &pt)) return nullptr;
    if (pt < 1 || pt > 6) { PyErr_SetString(PyExc_ValueError, "invalid piece type"); return nullptr; }
    return PyUnicode_FromString(PIECE_NAMES_ARR[pt]);
}
static PyObject* py_flip_diagonal(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromUnsignedLongLong(flip_diagonal(bb));
}
static PyObject* py_flip_anti_diagonal(PyObject*, PyObject* args) {
    unsigned long long bb;
    if (!PyArg_ParseTuple(args, "K", &bb)) return nullptr;
    return PyLong_FromUnsignedLongLong(flip_anti_diagonal(bb));
}

// ============================================================
// Module definition
// ============================================================

static PyMethodDef chess_methods[] = {

    {"parse_square",           py_parse_square,                 METH_VARARGS, "parse_square(name) -> int"},
    {"square_at",              py_square_at,                    METH_VARARGS, "square_at(file, rank) -> int"},
    {"file",                   py_file,                         METH_VARARGS, "file(square) -> int"},
    {"rank",                   py_rank,                         METH_VARARGS, "rank(square) -> int"},
    {"square_distance",        py_square_distance,              METH_VARARGS, "square_distance(a, b) -> int"},
    {"square_manhattan_distance", py_square_manhattan_distance, METH_VARARGS, "square_manhattan_distance(a, b) -> int"},
    {"square_knight_distance", py_square_knight_distance,       METH_VARARGS, "square_knight_distance(a, b) -> int"},
    {"mirror_square",          py_mirror_square,                METH_VARARGS, "mirror_square(square) -> int"},
    {"lsb",                    py_lsb,                          METH_VARARGS, "lsb(bitboard) -> int"},
    {"msb",                    py_msb,                          METH_VARARGS, "msb(bitboard) -> int"},
    {"scan_forward",           py_scan_forward,                 METH_VARARGS, "scan_forward(bitboard) -> list[int]"},
    {"scan_reversed",          py_scan_reversed,                METH_VARARGS, "scan_reversed(bitboard) -> list[int]"},
    {"ray",                    py_ray,                          METH_VARARGS, "ray(from, to) -> int (Bitboard)"},
    {"between",                py_between,                      METH_VARARGS, "between(from, to) -> int (Bitboard)"},
    {"flip_vertical",          py_flip_vertical,                METH_VARARGS, "flip_vertical(bb) -> int"},
    {"flip_horizontal",        py_flip_horizontal,              METH_VARARGS, "flip_horizontal(bb) -> int"},
    // python-chess exact-name aliases
    {"square_file",              py_square_file,                METH_VARARGS, "square_file(square) -> int"},
    {"square_rank",              py_square_rank,                METH_VARARGS, "square_rank(square) -> int"},
    {"square_mirror",            py_square_mirror,              METH_VARARGS, "square_mirror(square) -> int"},
    {"square_name",              py_square_name,                METH_VARARGS, "square_name(square) -> str"},
    {"square",                   py_square_function,            METH_VARARGS, "square(file_index, rank_index) -> int"},
    {"piece_symbol",             py_piece_symbol,               METH_VARARGS, "piece_symbol(piece_type) -> str"},
    {"piece_name",               py_piece_name,                 METH_VARARGS, "piece_name(piece_type) -> str"},
    {"flip_diagonal",            py_flip_diagonal,              METH_VARARGS, "flip_diagonal(bb) -> int"},
    {"flip_anti_diagonal",       py_flip_anti_diagonal,         METH_VARARGS, "flip_anti_diagonal(bb) -> int"},
        {nullptr}
};

static struct PyModuleDef chess_module = {
    PyModuleDef_HEAD_INIT,
    "libchess",
    "Python bindings for the chess C++ library",
    -1,
    chess_methods
};

#define CONTENT_NAME(name) "libchess" #name

PyMODINIT_FUNC PyInit_libchess(void) {
    // Assign all PyTypeObject fields here: designated initialisers cannot be
    // mixed with PyVarObject_HEAD_INIT() in C++ (it expands to positional
    // initialisers), so we set every field imperatively before PyType_Ready.

    PyOutcomeType.tp_name      = CONTENT_NAME(Outcome);
    PyOutcomeType.tp_basicsize = sizeof(PyOutcome);
    PyOutcomeType.tp_str       = (reprfunc)Outcome_str;
    PyOutcomeType.tp_flags     = Py_TPFLAGS_DEFAULT;
    PyOutcomeType.tp_doc       = "chess.Outcome — termination + winner";
    PyOutcomeType.tp_getset    = Outcome_getset;
    PyOutcomeType.tp_new       = PyType_GenericNew;

    PyPieceType.tp_name      = CONTENT_NAME(Piece);
    PyPieceType.tp_basicsize = sizeof(PyPiece);
    PyPieceType.tp_str       = (reprfunc)Piece_str;
    PyPieceType.tp_flags     = Py_TPFLAGS_DEFAULT;

    PyPieceType.tp_doc       = "chess.Piece(piece_type: int, color: int)";
    PyPieceType.tp_getset    = Piece_getset;
    PyPieceType.tp_init      = (initproc)Piece_init;
    PyPieceType.tp_new       = PyType_GenericNew;

    PyMoveType.tp_name      = CONTENT_NAME(Move);
    PyMoveType.tp_basicsize = sizeof(PyMove);
    PyMoveType.tp_str       = (reprfunc)Move_str;
    PyMoveType.tp_flags     = Py_TPFLAGS_DEFAULT;
    PyMoveType.tp_doc       = "chess.Move(from_square, to_square, promotion=None)";
    PyMoveType.tp_methods   = Move_methods;
    PyMoveType.tp_getset    = Move_getset;
    PyMoveType.tp_init      = (initproc)Move_init;
    PyMoveType.tp_new       = PyType_GenericNew;

    PyBaseBoardType.tp_name      = CONTENT_NAME(BaseBoard);
    PyBaseBoardType.tp_basicsize = sizeof(PyBaseBoard);
    PyBaseBoardType.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyBaseBoardType.tp_doc       = "chess.BaseBoard(board_fen=None)";
    PyBaseBoardType.tp_dealloc   = (destructor)BaseBoard_dealloc;
    PyBaseBoardType.tp_methods   = BaseBoard_methods;
    PyBaseBoardType.tp_getset    = BaseBoard_getset;
    PyBaseBoardType.tp_init      = (initproc)BaseBoard_init;
    PyBaseBoardType.tp_new       = PyType_GenericNew;

    PyBoardType.tp_name      = CONTENT_NAME(Board);
    PyBoardType.tp_basicsize = sizeof(PyBoard);
    PyBoardType.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyBoardType.tp_doc       = "chess.Board(fen=None) -- full chess board with move generation";
    PyBoardType.tp_dealloc   = (destructor)Board_dealloc;
    PyBoardType.tp_methods   = Board_methods;
    PyBoardType.tp_getset    = Board_getset;
    PyBoardType.tp_base      = &PyBaseBoardType;
    PyBoardType.tp_init      = (initproc)Board_init;
    PyBoardType.tp_new       = PyType_GenericNew;

    // Finalise types
    if (PyType_Ready(&PyOutcomeType)   < 0) return nullptr;
    if (PyType_Ready(&PyPieceType)     < 0) return nullptr;
    if (PyType_Ready(&PyMoveType)      < 0) return nullptr;
    if (PyType_Ready(&PyBaseBoardType) < 0) return nullptr;
    if (PyType_Ready(&PyBoardType)     < 0) return nullptr;

    PyObject* m = PyModule_Create(&chess_module);
    if (!m) return nullptr;

    // Add types
    Py_INCREF(&PyOutcomeType);
    PyModule_AddObject(m, "Outcome",   (PyObject*)&PyOutcomeType);
    Py_INCREF(&PyPieceType);
    PyModule_AddObject(m, "Piece",     (PyObject*)&PyPieceType);
    Py_INCREF(&PyMoveType);
    PyModule_AddObject(m, "Move",      (PyObject*)&PyMoveType);
    Py_INCREF(&PyBaseBoardType);
    PyModule_AddObject(m, "BaseBoard", (PyObject*)&PyBaseBoardType);
    Py_INCREF(&PyBoardType);
    PyModule_AddObject(m, "Board",     (PyObject*)&PyBoardType);

    // Termination constants (keep same integer values as python-chess enum.auto())
    PyModule_AddIntConstant(m, "CHECKMATE",             (int)(Termination::CHECKMATE));
    PyModule_AddIntConstant(m, "STALEMATE",             (int)(Termination::STALEMATE));
    PyModule_AddIntConstant(m, "INSUFFICIENT_MATERIAL", (int)(Termination::INSUFFICIENT_MATERIAL));
    PyModule_AddIntConstant(m, "SEVENTYFIVE_MOVES",     (int)(Termination::SEVENTYFIVE_MOVES));
    PyModule_AddIntConstant(m, "FIVEFOLD_REPETITION",   (int)(Termination::FIVEFOLD_REPETITION));
    PyModule_AddIntConstant(m, "FIFTY_MOVES",           (int)(Termination::FIFTY_MOVES));
    PyModule_AddIntConstant(m, "THREEFOLD_REPETITION",  (int)(Termination::THREEFOLD_REPETITION));
    PyModule_AddIntConstant(m, "VARIANT_WIN",           (int)(Termination::VARIANT_WIN));
    PyModule_AddIntConstant(m, "VARIANT_LOSS",          (int)(Termination::VARIANT_LOSS));
    PyModule_AddIntConstant(m, "VARIANT_DRAW",          (int)(Termination::VARIANT_DRAW));

    // ── PieceType constants ────────────────────────────────────────────────
    // python-chess: PAWN=1 KNIGHT=2 BISHOP=3 ROOK=4 QUEEN=5 KING=6
    PyModule_AddIntConstant(m, "PAWN",   (int)(PieceType::pawn));
    PyModule_AddIntConstant(m, "KNIGHT", (int)(PieceType::knight));
    PyModule_AddIntConstant(m, "BISHOP", (int)(PieceType::bishop));
    PyModule_AddIntConstant(m, "ROOK",   (int)(PieceType::rook));
    PyModule_AddIntConstant(m, "QUEEN",  (int)(PieceType::queen));
    PyModule_AddIntConstant(m, "KING",   (int)(PieceType::king));

    // PIECE_TYPES = [PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING]  (list of ints 1..6)
    {
        PyObject* lst = PyList_New(6);
        for (int i = 0; i < 6; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromLong(i + 1));
        PyModule_AddObject(m, "PIECE_TYPES", lst);
    }
    // PIECE_SYMBOLS = [None, "p", "n", "b", "r", "q", "k"]
    {
        const char* syms[] = { "", "p", "n", "b", "r", "q", "k" };
        PyObject* lst = PyList_New(7);
        PyList_SET_ITEM(lst, 0, Py_NewRef(Py_None));
        for (int i = 1; i <= 6; i++)
            PyList_SET_ITEM(lst, i, PyUnicode_FromString(syms[i]));
        PyModule_AddObject(m, "PIECE_SYMBOLS", lst);
    }
    // PIECE_NAMES = [None, "pawn", "knight", "bishop", "rook", "queen", "king"]
    {
        const char* names[] = { "", "pawn", "knight", "bishop", "rook", "queen", "king" };
        PyObject* lst = PyList_New(7);
        PyList_SET_ITEM(lst, 0, Py_NewRef(Py_None));
        for (int i = 1; i <= 6; i++)
            PyList_SET_ITEM(lst, i, PyUnicode_FromString(names[i]));
        PyModule_AddObject(m, "PIECE_NAMES", lst);
    }

    // ── Color constants ────────────────────────────────────────────────────
    // python-chess: WHITE=True(1), BLACK=False(0)
    PyModule_AddIntConstant(m, "WHITE", 1);
    PyModule_AddIntConstant(m, "BLACK", 0);
    // COLORS = [WHITE, BLACK]  →  [True, False]  (as ints: [1, 0])
    {
        PyObject* lst = PyList_New(2);
        PyList_SET_ITEM(lst, 0, PyBool_FromLong(1));  // WHITE
        PyList_SET_ITEM(lst, 1, PyBool_FromLong(0));  // BLACK
        PyModule_AddObject(m, "COLORS", lst);
    }
    // COLOR_NAMES = ["black", "white"]  (indexed by color value: 0=black, 1=white)
    {
        PyObject* lst = PyList_New(2);
        PyList_SET_ITEM(lst, 0, PyUnicode_FromString("black"));
        PyList_SET_ITEM(lst, 1, PyUnicode_FromString("white"));
        PyModule_AddObject(m, "COLOR_NAMES", lst);
    }

    // ── File / Rank name lists ─────────────────────────────────────────────
    // FILE_NAMES = ["a","b","c","d","e","f","g","h"]
    {
        const char* fn[] = {"a","b","c","d","e","f","g","h"};
        PyObject* lst = PyList_New(8);
        for (int i = 0; i < 8; i++) PyList_SET_ITEM(lst, i, PyUnicode_FromString(fn[i]));
        PyModule_AddObject(m, "FILE_NAMES", lst);
    }
    // RANK_NAMES = ["1","2","3","4","5","6","7","8"]
    {
        const char* rn[] = {"1","2","3","4","5","6","7","8"};
        PyObject* lst = PyList_New(8);
        for (int i = 0; i < 8; i++) PyList_SET_ITEM(lst, i, PyUnicode_FromString(rn[i]));
        PyModule_AddObject(m, "RANK_NAMES", lst);
    }

    // ── STARTING FEN strings ──────────────────────────────────────────────
    PyModule_AddStringConstant(m, "STARTING_FEN",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    PyModule_AddStringConstant(m, "STARTING_BOARD_FEN",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    // ── Square integer constants  A1=0 … H8=63 ───────────────────────────
#define ADD_SQ(name) PyModule_AddIntConstant(m, #name, (int)(Square::name))
    ADD_SQ(A1); ADD_SQ(B1); ADD_SQ(C1); ADD_SQ(D1);
    ADD_SQ(E1); ADD_SQ(F1); ADD_SQ(G1); ADD_SQ(H1);
    ADD_SQ(A2); ADD_SQ(B2); ADD_SQ(C2); ADD_SQ(D2);
    ADD_SQ(E2); ADD_SQ(F2); ADD_SQ(G2); ADD_SQ(H2);
    ADD_SQ(A3); ADD_SQ(B3); ADD_SQ(C3); ADD_SQ(D3);
    ADD_SQ(E3); ADD_SQ(F3); ADD_SQ(G3); ADD_SQ(H3);
    ADD_SQ(A4); ADD_SQ(B4); ADD_SQ(C4); ADD_SQ(D4);
    ADD_SQ(E4); ADD_SQ(F4); ADD_SQ(G4); ADD_SQ(H4);
    ADD_SQ(A5); ADD_SQ(B5); ADD_SQ(C5); ADD_SQ(D5);
    ADD_SQ(E5); ADD_SQ(F5); ADD_SQ(G5); ADD_SQ(H5);
    ADD_SQ(A6); ADD_SQ(B6); ADD_SQ(C6); ADD_SQ(D6);
    ADD_SQ(E6); ADD_SQ(F6); ADD_SQ(G6); ADD_SQ(H6);
    ADD_SQ(A7); ADD_SQ(B7); ADD_SQ(C7); ADD_SQ(D7);
    ADD_SQ(E7); ADD_SQ(F7); ADD_SQ(G7); ADD_SQ(H7);
    ADD_SQ(A8); ADD_SQ(B8); ADD_SQ(C8); ADD_SQ(D8);
    ADD_SQ(E8); ADD_SQ(F8); ADD_SQ(G8); ADD_SQ(H8);
#undef ADD_SQ

    // SQUARES = [0, 1, ..., 63]  (list — python-chess uses a list)
    {
        PyObject* lst = PyList_New(64);
        for (int i = 0; i < 64; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromLong(i));
        PyModule_AddObject(m, "SQUARES", lst);
    }
    // SQUARE_NAMES = ["a1","b1","c1",...,"h8"]  (rank-minor order, same as python-chess)
    {
        static const char* FILE_N = "abcdefgh";
        PyObject* lst = PyList_New(64);
        for (int sq = 0; sq < 64; sq++) {
            char name[3] = { FILE_N[sq & 7], (char)('1' + (sq >> 3)), 0 };
            PyList_SET_ITEM(lst, sq, PyUnicode_FromString(name));
        }
        PyModule_AddObject(m, "SQUARE_NAMES", lst);
    }
    // SQUARES_180 = [square_mirror(sq) for sq in SQUARES]  i.e. sq^0x38
    {
        PyObject* lst = PyList_New(64);
        for (int i = 0; i < 64; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromLong(i ^ 0x38));
        PyModule_AddObject(m, "SQUARES_180", lst);
    }

    // ── Bitboard constants ─────────────────────────────────────────────────
    PyModule_AddObject(m, "BB_EMPTY",         PyLong_FromUnsignedLongLong(BB_EMPTY));
    PyModule_AddObject(m, "BB_ALL",           PyLong_FromUnsignedLongLong(BB_ALL));
    PyModule_AddObject(m, "BB_LIGHT_SQUARES", PyLong_FromUnsignedLongLong(BB_LIGHT_SQUARES));
    PyModule_AddObject(m, "BB_DARK_SQUARES",  PyLong_FromUnsignedLongLong(BB_DARK_SQUARES));
    PyModule_AddObject(m, "BB_CORNERS",       PyLong_FromUnsignedLongLong(BB_CORNERS));
    PyModule_AddObject(m, "BB_CENTER",        PyLong_FromUnsignedLongLong(BB_CENTER));
    PyModule_AddObject(m, "BB_BACKRANKS",     PyLong_FromUnsignedLongLong(BB_BACKRANKS));

    // BB_SQUARES[sq] = 1ULL << sq  (list, indexed by square)
    {
        PyObject* lst = PyList_New(64);
        for (int i = 0; i < 64; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromUnsignedLongLong(1ULL << i));
        PyModule_AddObject(m, "BB_SQUARES", lst);
    }

    // Individual square bitboards  BB_A1..BB_H8
#define ADD_BB_SQ(name, sq) PyModule_AddObject(m, "BB_" #name, PyLong_FromUnsignedLongLong(1ULL << (int)(Square::sq)))
    ADD_BB_SQ(A1,A1); ADD_BB_SQ(B1,B1); ADD_BB_SQ(C1,C1); ADD_BB_SQ(D1,D1);
    ADD_BB_SQ(E1,E1); ADD_BB_SQ(F1,F1); ADD_BB_SQ(G1,G1); ADD_BB_SQ(H1,H1);
    ADD_BB_SQ(A2,A2); ADD_BB_SQ(B2,B2); ADD_BB_SQ(C2,C2); ADD_BB_SQ(D2,D2);
    ADD_BB_SQ(E2,E2); ADD_BB_SQ(F2,F2); ADD_BB_SQ(G2,G2); ADD_BB_SQ(H2,H2);
    ADD_BB_SQ(A3,A3); ADD_BB_SQ(B3,B3); ADD_BB_SQ(C3,C3); ADD_BB_SQ(D3,D3);
    ADD_BB_SQ(E3,E3); ADD_BB_SQ(F3,F3); ADD_BB_SQ(G3,G3); ADD_BB_SQ(H3,H3);
    ADD_BB_SQ(A4,A4); ADD_BB_SQ(B4,B4); ADD_BB_SQ(C4,C4); ADD_BB_SQ(D4,D4);
    ADD_BB_SQ(E4,E4); ADD_BB_SQ(F4,F4); ADD_BB_SQ(G4,G4); ADD_BB_SQ(H4,H4);
    ADD_BB_SQ(A5,A5); ADD_BB_SQ(B5,B5); ADD_BB_SQ(C5,C5); ADD_BB_SQ(D5,D5);
    ADD_BB_SQ(E5,E5); ADD_BB_SQ(F5,F5); ADD_BB_SQ(G5,G5); ADD_BB_SQ(H5,H5);
    ADD_BB_SQ(A6,A6); ADD_BB_SQ(B6,B6); ADD_BB_SQ(C6,C6); ADD_BB_SQ(D6,D6);
    ADD_BB_SQ(E6,E6); ADD_BB_SQ(F6,F6); ADD_BB_SQ(G6,G6); ADD_BB_SQ(H6,H6);
    ADD_BB_SQ(A7,A7); ADD_BB_SQ(B7,B7); ADD_BB_SQ(C7,C7); ADD_BB_SQ(D7,D7);
    ADD_BB_SQ(E7,E7); ADD_BB_SQ(F7,F7); ADD_BB_SQ(G7,G7); ADD_BB_SQ(H7,H7);
    ADD_BB_SQ(A8,A8); ADD_BB_SQ(B8,B8); ADD_BB_SQ(C8,C8); ADD_BB_SQ(D8,D8);
    ADD_BB_SQ(E8,E8); ADD_BB_SQ(F8,F8); ADD_BB_SQ(G8,G8); ADD_BB_SQ(H8,H8);
#undef ADD_BB_SQ

    // BB_FILES = [BB_FILE_A, ..., BB_FILE_H]  (list of 8 bitboards)
    {
        PyObject* lst = PyList_New(8);
        for (int i = 0; i < 8; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromUnsignedLongLong(0x0101010101010101ULL << i));
        PyModule_AddObject(m, "BB_FILES", lst);
    }
    // Individual BB_FILE_x
    PyModule_AddObject(m, "BB_FILE_A", PyLong_FromUnsignedLongLong(BB_FILE_A));
    PyModule_AddObject(m, "BB_FILE_B", PyLong_FromUnsignedLongLong(BB_FILE_B));
    PyModule_AddObject(m, "BB_FILE_C", PyLong_FromUnsignedLongLong(BB_FILE_C));
    PyModule_AddObject(m, "BB_FILE_D", PyLong_FromUnsignedLongLong(BB_FILE_D));
    PyModule_AddObject(m, "BB_FILE_E", PyLong_FromUnsignedLongLong(BB_FILE_E));
    PyModule_AddObject(m, "BB_FILE_F", PyLong_FromUnsignedLongLong(BB_FILE_F));
    PyModule_AddObject(m, "BB_FILE_G", PyLong_FromUnsignedLongLong(BB_FILE_G));
    PyModule_AddObject(m, "BB_FILE_H", PyLong_FromUnsignedLongLong(BB_FILE_H));

    // BB_RANKS = [BB_RANK_1, ..., BB_RANK_8]  (list of 8 bitboards)
    {
        PyObject* lst = PyList_New(8);
        for (int i = 0; i < 8; i++)
            PyList_SET_ITEM(lst, i, PyLong_FromUnsignedLongLong(0xFFULL << (8 * i)));
        PyModule_AddObject(m, "BB_RANKS", lst);
    }
    // Individual BB_RANK_x
    PyModule_AddObject(m, "BB_RANK_1", PyLong_FromUnsignedLongLong(BB_RANK_1));
    PyModule_AddObject(m, "BB_RANK_2", PyLong_FromUnsignedLongLong(BB_RANK_2));
    PyModule_AddObject(m, "BB_RANK_3", PyLong_FromUnsignedLongLong(BB_RANK_3));
    PyModule_AddObject(m, "BB_RANK_4", PyLong_FromUnsignedLongLong(BB_RANK_4));
    PyModule_AddObject(m, "BB_RANK_5", PyLong_FromUnsignedLongLong(BB_RANK_5));
    PyModule_AddObject(m, "BB_RANK_6", PyLong_FromUnsignedLongLong(BB_RANK_6));
    PyModule_AddObject(m, "BB_RANK_7", PyLong_FromUnsignedLongLong(BB_RANK_7));
    PyModule_AddObject(m, "BB_RANK_8", PyLong_FromUnsignedLongLong(BB_RANK_8));

    return m;
}