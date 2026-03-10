/*
Derived from python-chess
Copyright (C) 2012-2021 Niklas Fiekas

Translated to C++ by typeulli (2026)
Licensed under GPL v3
*/

#pragma once
#include "libchess.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <tuple>

NAMESPACE_BEGIN

// ─────────────────────────────────────────────
// Board constructor
// ─────────────────────────────────────────────

const std::string Board::starting_fen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

inline Board::Board(std::optional<std::string> fen)
    : BaseBoard(std::nullopt)
{
    ep_square = std::nullopt;
    move_stack.clear();
    _stack.clear();

    if (!fen.has_value()) {
        clear();
    } else if (*fen == starting_fen) {
        reset();
    } else {
        set_fen(*fen);
    }
}

// ─────────────────────────────────────────────
// Reset / Clear
// ─────────────────────────────────────────────

inline void Board::reset() {
    turn = Color::white;
    ep_square = std::nullopt;
    halfmove_clock = 0;
    fullmove_number = 1;
    reset_board();
    castling_rights = BB_CORNERS; // set after reset_board() so it is not overwritten
}

inline void Board::reset_board() {
    BaseBoard::reset_board();
    clear_stack();
}

inline void Board::clear() {
    turn = Color::white;
    ep_square = std::nullopt;
    halfmove_clock = 0;
    fullmove_number = 1;
    clear_board();
    castling_rights = BB_EMPTY;
}

inline void Board::clear_board() {
    BaseBoard::clear_board();
    clear_stack();
}

inline void Board::clear_stack() {
    move_stack.clear();
    _stack.clear();
}

// ─────────────────────────────────────────────
// Root / Ply
// ─────────────────────────────────────────────

inline Board Board::root() const {
    if (!_stack.empty()) {
        Board b(std::nullopt);
        _stack.front().restore(b);
        return b;
    }
    return copy(false);
}

inline int Board::ply() const {
    return 2 * (fullmove_number - 1) + (turn == Color::black ? 1 : 0);
}

// ─────────────────────────────────────────────
// Piece manipulation (clears stack)
// ─────────────────────────────────────────────

inline std::optional<Piece> Board::remove_piece_at(Square square) {
    auto piece = BaseBoard::remove_piece_at(square);
    clear_stack();
    return piece;
}

inline void Board::set_piece_at(Square square, std::optional<Piece> piece, bool promoted) {
    BaseBoard::set_piece_at(square, piece, promoted);
    clear_stack();
}

// ─────────────────────────────────────────────
// Pseudo-legal move generation
// ─────────────────────────────────────────────

inline vector<Move> Board::generate_pseudo_legal_moves(Bitboard from_mask, Bitboard to_mask) const {
    vector<Move> moves;
    Bitboard our_pieces = occupied_co[intOf(turn)];

    // Non-pawn pieces
    Bitboard non_pawns = our_pieces & ~pawns & from_mask;
    for (Square from_sq : scan_reversed(non_pawns)) {
        Bitboard targets = attacks_mask(from_sq)
                       & ~our_pieces
                       & ~(kings & occupied_co[intOf(~turn)])
                       & to_mask;
        for (Square to_sq : scan_reversed(targets))
            moves.emplace_back(from_sq, to_sq);
    }

    // Castling
    if (from_mask & kings) {
        auto castling = generate_castling_moves(from_mask, to_mask);
        moves.insert(moves.end(), castling.begin(), castling.end());
    }

    // Pawns
    Bitboard pawns_bb = pawns & occupied_co[intOf(turn)] & from_mask;
    if (!pawns_bb) return moves;

    // Pawn captures
    for (Square from_sq : scan_reversed(pawns_bb)) {
        Bitboard targets = BB_PAWN_ATTACKS[intOf(turn)][intOf(from_sq)]
                           & occupied_co[intOf(~turn)] & to_mask;
        for (Square to_sq : scan_reversed(targets)) {
            int r = rank(to_sq);
            if (r == 0 || r == 7) {
                moves.emplace_back(from_sq, to_sq, PieceType::queen);
                moves.emplace_back(from_sq, to_sq, PieceType::rook);
                moves.emplace_back(from_sq, to_sq, PieceType::bishop);
                moves.emplace_back(from_sq, to_sq, PieceType::knight);
            } else {
                moves.emplace_back(from_sq, to_sq);
            }
        }
    }

    // Single and double pawn advances
    Bitboard single_moves, double_moves;
    if (turn == Color::white) {
        single_moves = (pawns_bb << 8) & ~occupied;
        double_moves = (single_moves << 8) & ~occupied & (BB_RANK_3 | BB_RANK_4);
    } else {
        single_moves = (pawns_bb >> 8) & ~occupied;
        double_moves = (single_moves >> 8) & ~occupied & (BB_RANK_6 | BB_RANK_5);
    }
    single_moves &= to_mask;
    double_moves &= to_mask;

    for (Square to_sq : scan_reversed(single_moves)) {
        Square from_sq = (Square)(intOf(to_sq) + (turn == Color::black ? 8 : -8));
        int r = rank(to_sq);
        if (r == 0 || r == 7) {
            moves.emplace_back(from_sq, to_sq, PieceType::queen);
            moves.emplace_back(from_sq, to_sq, PieceType::rook);
            moves.emplace_back(from_sq, to_sq, PieceType::bishop);
            moves.emplace_back(from_sq, to_sq, PieceType::knight);
        } else {
            moves.emplace_back(from_sq, to_sq);
        }
    }

    for (Square to_sq : scan_reversed(double_moves)) {
        Square from_sq = (Square)(intOf(to_sq) + (turn == Color::black ? 16 : -16));
        moves.emplace_back(from_sq, to_sq);
    }

    // En passant
    auto ep = generate_pseudo_legal_ep(from_mask, to_mask);
    moves.insert(moves.end(), ep.begin(), ep.end());

    return moves;
}

inline vector<Move> Board::generate_pseudo_legal_ep(Bitboard from_mask, Bitboard to_mask) const {
    vector<Move> moves;
    if (!ep_square) return moves;
    if (!(BB_SQUARES(*ep_square) & to_mask)) return moves;
    if (BB_SQUARES(*ep_square) & occupied) return moves;

    int turn_int = intOf(turn);
    Bitboard capturers = pawns & occupied_co[turn_int] & from_mask
        & BB_PAWN_ATTACKS[intOf(~turn)][intOf(*ep_square)]
        & BB_RANKS[turn == Color::white ? 4 : 3];

    for (Square capturer : scan_reversed(capturers))
        moves.emplace_back(capturer, *ep_square);

    return moves;
}

inline vector<Move> Board::generate_pseudo_legal_captures(Bitboard from_mask, Bitboard to_mask) const {
    auto a = generate_pseudo_legal_moves(from_mask, to_mask & occupied_co[intOf(~turn)]);
    auto b = generate_pseudo_legal_ep(from_mask, to_mask);
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

// ─────────────────────────────────────────────
// Check detection
// ─────────────────────────────────────────────

inline Bitboard Board::checkers_mask() const {
    auto k = king(turn);
    if (!k) return BB_EMPTY;
    return attackers_mask(~turn, *k);
}

inline bool Board::is_check() const { return (bool)(checkers_mask()); }

inline bool Board::gives_check(const Move& move) {
    push(const_cast<Move&>(move));
    bool result = is_check();
    pop();
    return result;
}

inline bool Board::is_into_check(const Move& move) const {
    auto k = king(turn);
    if (!k) return false;

    Bitboard checkers_bb = attackers_mask(~turn, *k);
    if (checkers_bb) {
        auto evasions = _generate_evasions(*k, checkers_bb,
            BB_SQUARES(move.from), BB_SQUARES(move.to));
        bool found = false;
        for (auto& m : evasions)
            if (m.from == move.from && m.to == move.to && m.promotion == move.promotion) { found = true; break; }
        if (!found) return true;
    }
    return !_is_safe(*k, _slider_blockers(*k), move);
}

inline bool Board::was_into_check() const {
    auto k = king(~turn);
    return k.has_value() && is_attacked_by(turn, *k);
}

// ─────────────────────────────────────────────
// Legality
// ─────────────────────────────────────────────

inline bool Board::is_pseudo_legal(const Move& move) const {
    // Null move
    if (move.from == move.to && !move.promotion && !move.drop) return false;
    if (move.drop) return false;

    auto piece = piece_type_at(move.from);
    if (!piece) return false;

    Bitboard from_bb = BB_SQUARES(move.from);
    Bitboard to_bb   = BB_SQUARES(move.to);

    if (!(occupied_co[intOf(turn)] & from_bb)) return false;

    if (move.promotion) {
        if (*piece != PieceType::pawn) return false;
        if (turn == Color::white && rank(move.to) != 7) return false;
        if (turn == Color::black && rank(move.to) != 0) return false;
    }

    if (*piece == PieceType::king) {
        Move m960 = _from_chess960(false, move.from, move.to, move.promotion, move.drop);
        auto castling = generate_castling_moves();
        for (auto& c : castling)
            if (c.from == m960.from && c.to == m960.to) return true;
    }

    if (occupied_co[intOf(turn)] & to_bb) return false;

    if (*piece == PieceType::pawn) {
        auto pawn_moves = generate_pseudo_legal_moves(from_bb, to_bb);
        for (auto& m : pawn_moves)
            if (m.from == move.from && m.to == move.to && m.promotion == move.promotion)
                return true;
        return false;
    }

    return (bool)(attacks_mask(move.from) & to_bb);
}

inline bool Board::is_legal(const Move& move) const {
    return !is_variant_end() && is_pseudo_legal(move) && !is_into_check(move);
}

// ─────────────────────────────────────────────
// Variant stubs (standard chess → always false)
// ─────────────────────────────────────────────

inline bool Board::is_variant_end()  const { return false; }
inline bool Board::is_variant_loss() const { return false; }
inline bool Board::is_variant_win()  const { return false; }
inline bool Board::is_variant_draw() const { return false; }

// ─────────────────────────────────────────────
// Game-over / outcome
// ─────────────────────────────────────────────

inline bool Board::is_game_over(bool claim_draw) {
    return outcome(claim_draw).has_value();
}

inline std::string Board::result(bool claim_draw) {
    auto o = outcome(claim_draw);
    if (!o) return "*";
    switch (o->termination) {
        case Termination::CHECKMATE:
        case Termination::VARIANT_WIN:
        case Termination::VARIANT_LOSS:
            return (o->winner == Color::white) ? "1-0" : "0-1";
        default:
            return "1/2-1/2";
    }
}

inline std::optional<Outcome> Board::outcome(bool claim_draw) {
    // Color::white is used as a placeholder winner for draws (winner field is unused).
    if (is_variant_loss()) return Outcome{Termination::VARIANT_LOSS, ~turn};
    if (is_variant_win())  return Outcome{Termination::VARIANT_WIN,  turn};
    if (is_variant_draw()) return Outcome{Termination::VARIANT_DRAW, Color::white};

    // Cache legal moves to avoid redundant generation inside is_checkmate/is_stalemate.
    if (is_check()) {
        auto legal = generate_legal_moves();
        if (legal.empty()) return Outcome{Termination::CHECKMATE, ~turn};
    } else {
        if (is_insufficient_material()) return Outcome{Termination::INSUFFICIENT_MATERIAL, Color::white};
        auto legal = generate_legal_moves();
        if (legal.empty()) return Outcome{Termination::STALEMATE, Color::white};
    }

    if (is_seventyfive_moves())    return Outcome{Termination::SEVENTYFIVE_MOVES, Color::white};
    if (is_fivefold_repetition())  return Outcome{Termination::FIVEFOLD_REPETITION, Color::white};

    if (claim_draw) {
        if (can_claim_fifty_moves())          return Outcome{Termination::FIFTY_MOVES, Color::white};
        if (can_claim_threefold_repetition()) return Outcome{Termination::THREEFOLD_REPETITION, Color::white};
    }
    return std::nullopt;
}

inline bool Board::is_checkmate() const {
    if (!is_check()) return false;
    return generate_legal_moves().empty();
}

inline bool Board::is_stalemate() const {
    if (is_check()) return false;
    if (is_variant_end()) return false;
    return generate_legal_moves().empty();
}

inline bool Board::is_insufficient_material() const {
    return has_insufficient_material(Color::white) && has_insufficient_material(Color::black);
}

inline bool Board::has_insufficient_material(Color color) const {
    int c = intOf(color);
    if (occupied_co[c] & (pawns | rooks | queens)) return false;

    if (occupied_co[c] & knights) {
        return (std::popcount(occupied_co[c]) <= 2) &&
               !(occupied_co[intOf(~color)] & ~kings & ~queens);
    }

    if (occupied_co[c] & bishops) {
        bool same_color = !(bishops & BB_DARK_SQUARES) || !(bishops & BB_LIGHT_SQUARES);
        return same_color && !pawns && !knights;
    }

    return true;
}

// ─────────────────────────────────────────────
// Draw conditions
// ─────────────────────────────────────────────

inline bool Board::_is_halfmoves(int n) const {
    return halfmove_clock >= n && !generate_legal_moves().empty();
}

inline bool Board::is_seventyfive_moves() const { return _is_halfmoves(150); }
inline bool Board::is_fivefold_repetition() const { return is_repetition(5); }

inline bool Board::can_claim_draw() {
    return can_claim_fifty_moves() || can_claim_threefold_repetition();
}

inline bool Board::is_fifty_moves() const { return _is_halfmoves(100); }

inline bool Board::can_claim_fifty_moves() {
    if (is_fifty_moves()) return true;
    if (halfmove_clock >= 99) {
        for (auto& move : generate_legal_moves()) {
            if (!is_zeroing(move)) {
                push(move);
                bool ok = is_fifty_moves();
                pop();
                if (ok) return true;
            }
        }
    }
    return false;
}

inline bool Board::can_claim_threefold_repetition() {
    auto key = _transposition_key();
    std::map<decltype(key), int> transpositions;
    transpositions[key]++;

    vector<Move> switchyard;
    while (!move_stack.empty()) {
        Move m = pop();
        switchyard.push_back(m);
        if (is_irreversible(m)) break;
        transpositions[_transposition_key()]++;
    }
    while (!switchyard.empty()) { push(switchyard.back()); switchyard.pop_back(); }

    if (transpositions[key] >= 3) return true;

    for (auto& move : generate_legal_moves()) {
        push(move);
        auto it = transpositions.find(_transposition_key());
        bool ok = (it != transpositions.end() && it->second >= 2);
        pop();
        if (ok) return true;
    }
    return false;
}

inline bool Board::is_repetition(int count) const {
    int maybe = 1;
    for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
        if (it->occupied == occupied) {
            ++maybe;
            if (maybe >= count) break;
        }
    }
    if (maybe < count) return false;

    auto key = _transposition_key();
    vector<Move> switchyard;
    Board* self = const_cast<Board*>(this);

    int remaining = count;
    bool result = false;

    while (true) {
        if (remaining <= 1) { result = true; break; }
        if ((int)(self->move_stack.size()) < remaining - 1) break;
        Move m = self->pop();
        switchyard.push_back(m);
        if (self->is_irreversible(m)) break;
        if (self->_transposition_key() == key) --remaining;
    }
    while (!switchyard.empty()) { self->push(switchyard.back()); switchyard.pop_back(); }
    return result;
}

// ─────────────────────────────────────────────
// Push / Pop / Peek
// ─────────────────────────────────────────────

inline BoardState Board::_board_state() const { return BoardState(*this); }

inline void Board::_push_capture(const Move&, Square, PieceType, bool) {}

inline void Board::push(Move move) {
    move = _to_chess960(move);
    BoardState state = _board_state();
    castling_rights = clean_castling_rights();

    move_stack.push_back(_from_chess960(false, move.from, move.to, move.promotion, move.drop));
    _stack.push_back(state);

    std::optional<Square> ep_sq = ep_square;
    ep_square = std::nullopt;

    halfmove_clock++;
    if (turn == Color::black) fullmove_number++;

    // Null move
    if (move.from == move.to && !move.promotion && !move.drop) {
        turn = ~turn;
        return;
    }

    // Drop
    if (move.drop) {
        _set_piece_at(move.to, *move.drop, turn);
        turn = ~turn;
        return;
    }

    if (is_zeroing(move)) halfmove_clock = 0;

    Bitboard from_bb = BB_SQUARES(move.from);
    Bitboard to_bb   = BB_SQUARES(move.to);

    bool promoted_piece = (bool)(promoted & from_bb);
    auto piece_type = _remove_piece_at(move.from);
    assert(piece_type.has_value());

    Square capture_square = move.to;
    auto captured_piece_type = piece_type_at(capture_square);

    // Update castling rights
    castling_rights &= ~to_bb & ~from_bb;
    if (*piece_type == PieceType::king && !promoted_piece) {
        if (turn == Color::white) castling_rights &= ~BB_RANK_1;
        else                      castling_rights &= ~BB_RANK_8;
    } else if (captured_piece_type && *captured_piece_type == PieceType::king
               && !(promoted & to_bb)) {
        if (turn == Color::white && rank(move.to) == 7) castling_rights &= ~BB_RANK_8;
        if (turn == Color::black && rank(move.to) == 0) castling_rights &= ~BB_RANK_1;
    }

    // Special pawn handling
    if (*piece_type == PieceType::pawn) {
        int diff = intOf(move.to) - intOf(move.from);
        if (diff == 16 && rank(move.from) == 1) {
            ep_square = (Square)(intOf(move.from) + 8);
        } else if (diff == -16 && rank(move.from) == 6) {
            ep_square = (Square)(intOf(move.from) - 8);
        } else if (ep_sq && move.to == *ep_sq && (diff == 7 || diff == -7 || diff == 9 || diff == -9)
                   && !captured_piece_type) {
            int down = (turn == Color::white) ? -8 : 8;
            capture_square = (Square)(intOf(*ep_sq) + down);
            captured_piece_type = _remove_piece_at(capture_square);
        }
    }

    // Promotion
    if (move.promotion) {
        promoted_piece = true;
        piece_type = move.promotion;
    }

    // Castling
    bool castling = (*piece_type == PieceType::king) && (occupied_co[intOf(turn)] & to_bb);
    if (castling) {
        bool a_side = file(move.to) < file(move.from);
        _remove_piece_at(move.from);
        _remove_piece_at(move.to);
        if (a_side) {
            _set_piece_at(turn == Color::white ? Square::C1 : Square::C8, PieceType::king, turn);
            _set_piece_at(turn == Color::white ? Square::D1 : Square::D8, PieceType::rook, turn);
        } else {
            _set_piece_at(turn == Color::white ? Square::G1 : Square::G8, PieceType::king, turn);
            _set_piece_at(turn == Color::white ? Square::F1 : Square::F8, PieceType::rook, turn);
        }
    }

    if (!castling) {
        bool was_promoted = (bool)(this->promoted & to_bb);
        _set_piece_at(move.to, *piece_type, turn, promoted_piece);
        if (captured_piece_type)
            _push_capture(move, capture_square, *captured_piece_type, was_promoted);
    }

    turn = ~turn;
}

inline Move Board::pop() {
    Move m = move_stack.back(); move_stack.pop_back();
    _stack.back().restore(*this);
    _stack.pop_back();
    return m;
}

inline Move Board::peek() const {
    return move_stack.back();
}

// ─────────────────────────────────────────────
// find_move
// ─────────────────────────────────────────────

inline Move Board::find_move(Square from_sq, Square to_sq, std::optional<PieceType> promotion) const {
    if (!promotion && (pawns & BB_SQUARES(from_sq)) && (BB_SQUARES(to_sq) & BB_BACKRANKS))
        promotion = PieceType::queen;

    Move move = _from_chess960(false, from_sq, to_sq, promotion);
    if (!is_legal(move))
        throw std::invalid_argument("no matching legal move");
    return move;
}

// ─────────────────────────────────────────────
// Castling FEN helpers
// ─────────────────────────────────────────────

inline std::string Board::castling_shredder_fen() const {
    Bitboard cr = clean_castling_rights();
    if (!cr) return "-";
    std::string s;
    // scan_reversed: from higher bits (h-file) to lower bits (a-file)
    for (Square sq : scan_reversed(cr & BB_RANK_1))
        s += (char)('A' + file(sq));
    for (Square sq : scan_reversed(cr & BB_RANK_8))
        s += (char)('a' + file(sq));
    return s;
}

inline std::string Board::castling_xfen() const {
    std::string s;
    for (Color color : {Color::white, Color::black}) {
        auto k = king(color);
        if (!k) continue;

        int king_file_idx = file(*k);
        Bitboard backrank = (color == Color::white) ? BB_RANK_1 : BB_RANK_8;
        Bitboard cr = clean_castling_rights() & backrank;

        // Scan from h-file to a-file (kingside first)
        for (Square rook_sq : scan_reversed(cr)) {
            int rook_file_idx = file(rook_sq);
            bool a_side = rook_file_idx < king_file_idx;

            Bitboard other_rooks = occupied_co[intOf(color)] & rooks & backrank
                                   & ~BB_SQUARES(rook_sq);
            bool ambiguous = false;
            for (Square other : scan_reversed(other_rooks))
                if ((file(other) < rook_file_idx) == a_side) { ambiguous = true; break; }

            // Use file letter if ambiguous, otherwise 'k'/'q'.
            char ch = ambiguous ? (char)('a' + rook_file_idx) : (a_side ? 'q' : 'k');
            s += (color == Color::white) ? (char)(toupper(ch)) : ch;
        }
    }
    return s.empty() ? "-" : s;
}

inline bool Board::has_pseudo_legal_en_passant() const {
    return ep_square.has_value() && !generate_pseudo_legal_ep().empty();
}

inline bool Board::has_legal_en_passant() const {
    return ep_square.has_value() && !generate_legal_ep().empty();
}

// ─────────────────────────────────────────────
// FEN
// ─────────────────────────────────────────────

inline std::string Board::fen(bool shredder, const std::string& en_passant, std::optional<bool> promoted_flag) const {
    return epd(shredder, en_passant, promoted_flag)
        + " " + std::to_string(halfmove_clock)
        + " " + std::to_string(fullmove_number);
}

inline std::string Board::shredder_fen(const std::string& en_passant, std::optional<bool> promoted_flag) const {
    return fen(true, en_passant, promoted_flag);
}

inline void Board::set_fen(const std::string& fen_str) {
    std::istringstream ss(fen_str);
    std::vector<std::string> parts;
    std::string tok;
    while (ss >> tok) parts.push_back(tok);

    if (parts.empty()) throw std::invalid_argument("empty fen");

    std::string board_part = parts[0];

    Color turn_val = Color::white;
    if (parts.size() > 1) {
        if (parts[1] == "w") turn_val = Color::white;
        else if (parts[1] == "b") turn_val = Color::black;
        else throw std::invalid_argument("invalid turn in fen");
    }

    std::string castling_part = (parts.size() > 2) ? parts[2] : "-";

    std::optional<Square> ep_sq;
    if (parts.size() > 3 && parts[3] != "-") {
        ep_sq = parse_square(parts[3].c_str());
    }

    int hmc = 0;
    if (parts.size() > 4) hmc = std::stoi(parts[4]);
    if (hmc < 0) throw std::invalid_argument("negative halfmove clock");

    int fmn = 1;
    if (parts.size() > 5) fmn = std::max(1, std::stoi(parts[5]));

    _set_board_fen(board_part);
    turn = turn_val;
    _set_castling_fen(castling_part);
    ep_square = ep_sq;
    halfmove_clock = hmc;
    fullmove_number = fmn;
    clear_stack();
}

inline void Board::_set_castling_fen(const std::string& castling_fen) {
    if (castling_fen.empty() || castling_fen == "-") {
        castling_rights = BB_EMPTY;
        return;
    }

    castling_rights = BB_EMPTY;

    for (char flag : castling_fen) {
        Color color = isupper(flag) ? Color::white : Color::black;
        char f = tolower(flag);
        Bitboard backrank = (color == Color::white) ? BB_RANK_1 : BB_RANK_8;
        Bitboard rooks_bb = occupied_co[intOf(color)] & rooks & backrank;
        auto k = king(color);

        if (f == 'q') {
            // Queenside: pick the rook closest to the a-file.
            if (k) {
                Bitboard lsb_rook = rooks_bb & (~rooks_bb + 1); // lsb
                if (lsb_rook && msb(lsb_rook) < intOf(*k)) {
                    castling_rights |= lsb_rook;
                } else {
                    castling_rights |= BB_FILE_A & backrank;
                }
            } else {
                castling_rights |= BB_FILE_A & backrank;
            }
        } else if (f == 'k') {
            // Kingside: pick the rook closest to the h-file.
            if (k && rooks_bb) {
                int rook_sq = msb(rooks_bb);
                if (intOf(*k) < rook_sq) {
                    castling_rights |= BB_SQUARES((Square)(rook_sq));
                } else {
                    castling_rights |= BB_FILE_H & backrank;
                }
            } else {
                castling_rights |= BB_FILE_H & backrank;
            }
        } else {
            // Shredder FEN: file letter directly specifies the rook.
            int file_idx = f - 'a';
            castling_rights |= BB_FILES[file_idx] & backrank;
        }
    }
}

inline void Board::set_castling_fen(const std::string& castling_fen) {
    _set_castling_fen(castling_fen);
    clear_stack();
}

inline void Board::set_board_fen(const std::string& fen_str) {
    BaseBoard::set_board_fen(fen_str);
    clear_stack();
}

inline void Board::set_piece_map(const std::map<Square, Piece>& pieces) {
    BaseBoard::set_piece_map(pieces);
    clear_stack();
}

// ─────────────────────────────────────────────
// EPD
// ─────────────────────────────────────────────

inline std::string Board::epd(bool shredder, const std::string& en_passant, std::optional<bool> promoted_flag) const {
    std::optional<Square> ep_sq;
    if (en_passant == "fen")       ep_sq = ep_square;
    else if (en_passant == "xfen") ep_sq = has_pseudo_legal_en_passant() ? ep_square : std::nullopt;
    else                           ep_sq = has_legal_en_passant() ? ep_square : std::nullopt;

    std::string board_str;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Square sq = at(f, r);
            auto pt = piece_type_at(sq);
            if (!pt) { ++empty; continue; }
            if (empty) { board_str += std::to_string(empty); empty = 0; }
            auto col = color_at(sq);
            char c;
            switch (*pt) {
                case PieceType::pawn:   c = 'p'; break;
                case PieceType::knight: c = 'n'; break;
                case PieceType::bishop: c = 'b'; break;
                case PieceType::rook:   c = 'r'; break;
                case PieceType::queen:  c = 'q'; break;
                case PieceType::king:   c = 'k'; break;
                default:                c = '?'; break;
            }
            if (*col == Color::white) c = toupper(c);
            board_str += c;
        }
        if (empty) board_str += std::to_string(empty);
        if (r > 0) board_str += '/';
    }

    std::string turn_str = (turn == Color::white) ? "w" : "b";
    std::string castling_str = shredder ? castling_shredder_fen() : castling_xfen();
    std::string ep_str = ep_sq
        ? std::string(1, 'a' + file(*ep_sq)) + std::to_string(rank(*ep_sq) + 1)
        : "-";

    return board_str + " " + turn_str + " " + castling_str + " " + ep_str;
}

inline std::map<std::string, std::string> Board::set_epd(const std::string& epd_str) {
    std::istringstream ss(epd_str);
    std::vector<std::string> parts;
    std::string tok;
    while (ss >> tok) parts.push_back(tok);

    if (parts.size() >= 4) {
        std::string fen_str = parts[0] + " " + parts[1] + " " + parts[2] + " " + parts[3] + " 0 1";
        set_fen(fen_str);
    }
    return {};
}

// ─────────────────────────────────────────────
// SAN / algebraic notation
// ─────────────────────────────────────────────

static const char* piece_char(PieceType pt) {
    switch (pt) {
        case PieceType::knight: return "N";
        case PieceType::bishop: return "B";
        case PieceType::rook:   return "R";
        case PieceType::queen:  return "Q";
        case PieceType::king:   return "K";
        default:                return "";
    }
}

inline std::string Board::_algebraic_without_suffix(const Move& move, bool long_notation) const {
    // Null move
    if (move.from == move.to && !move.promotion) return "--";

    // Castling
    if (is_castling(move)) {
        return (file(move.to) < file(move.from)) ? "O-O-O" : "O-O";
    }

    auto pt = piece_type_at(move.from);
    assert(pt.has_value());
    bool cap = is_capture(move);

    std::string san;
    if (*pt != PieceType::pawn) san += piece_char(*pt);

    if (long_notation) {
        san += (char)('a' + file(move.from));
        san += (char)('1' + rank(move.from));
    } else if (*pt != PieceType::pawn) {
        // Disambiguation
        Bitboard others = 0;
        Bitboard from_mask = pieces_mask(*pt, turn) & ~BB_SQUARES(move.from);
        Bitboard to_mask   = BB_SQUARES(move.to);
        for (auto& cand : generate_legal_moves(from_mask, to_mask))
            others |= BB_SQUARES(cand.from);

        if (others) {
            bool row = false, col = false;
            if (others & BB_RANKS[rank(move.from)]) col = true;
            if (others & BB_FILES[file(move.from)]) row = true;
            else col = true;
            if (col) san += (char)('a' + file(move.from));
            if (row) san += (char)('1' + rank(move.from));
        }
    } else if (cap) {
        san += (char)('a' + file(move.from));
    }

    if (cap) san += 'x';
    else if (long_notation) san += '-';

    san += (char)('a' + file(move.to));
    san += (char)('1' + rank(move.to));

    if (move.promotion) {
        san += '=';
        san += piece_char(*move.promotion);
    }

    return san;
}

inline std::string Board::_algebraic_and_push(const Move& move, bool long_notation) {
    std::string san = _algebraic_without_suffix(move, long_notation);
    push(const_cast<Move&>(move));
    bool check = is_check();
    bool checkmate = (check && is_checkmate()) || is_variant_loss() || is_variant_win();
    if (checkmate) return san + "#";
    if (check)     return san + "+";
    return san;
}

inline std::string Board::_algebraic(const Move& move, bool long_notation) {
    std::string san = _algebraic_and_push(move, long_notation);
    pop();
    return san;
}

inline std::string Board::san(const Move& move)  { return _algebraic(move); }
inline std::string Board::lan(const Move& move)  { return _algebraic(move, true); }
inline std::string Board::san_and_push(const Move& move) { return _algebraic_and_push(move); }

inline std::string Board::variation_san(const vector<Move>& variation) const {
    Board b = copy(false);
    std::string result_str;
    bool first = true;
    for (auto& move : variation) {
        if (!b.is_legal(move)) throw std::invalid_argument("illegal move in variation");
        if (!first) result_str += ' ';
        first = false;
        if (b.turn == Color::white)
            result_str += std::to_string(b.fullmove_number) + ". ";
        else if (result_str.empty())
            result_str += std::to_string(b.fullmove_number) + "...";
        result_str += b.san_and_push(move);
    }
    return result_str;
}

// ─────────────────────────────────────────────
// parse_san
// ─────────────────────────────────────────────

inline Move Board::parse_san(const std::string& san_str) const {
    // Castling
    if (san_str == "O-O" || san_str == "O-O+" || san_str == "O-O#" ||
        san_str == "0-0" || san_str == "0-0+" || san_str == "0-0#") {
        for (auto& m : generate_castling_moves())
            if (is_kingside_castling(m)) return m;
        throw std::invalid_argument("illegal san: " + san_str);
    }
    if (san_str == "O-O-O" || san_str == "O-O-O+" || san_str == "O-O-O#" ||
        san_str == "0-0-0" || san_str == "0-0-0+" || san_str == "0-0-0#") {
        for (auto& m : generate_castling_moves())
            if (is_queenside_castling(m)) return m;
        throw std::invalid_argument("illegal san: " + san_str);
    }
    // Null
    if (san_str == "--" || san_str == "Z0" || san_str == "0000" || san_str == "@@@@")
        return {Square::A1, Square::A1};

    // Strip suffix
    std::string s = san_str;
    while (!s.empty() && (s.back() == '+' || s.back() == '#')) s.pop_back();

    // Parse piece type
    PieceType piece_type = PieceType::pawn;
    int i = 0;
    if (!s.empty() && isupper(s[0]) && s[0] != 'O') {
        switch (s[0]) {
            case 'N': piece_type = PieceType::knight; break;
            case 'B': piece_type = PieceType::bishop; break;
            case 'R': piece_type = PieceType::rook;   break;
            case 'Q': piece_type = PieceType::queen;  break;
            case 'K': piece_type = PieceType::king;   break;
        }
        if (piece_type != PieceType::pawn) i++;
    }

    // Optional promotion
    std::optional<PieceType> promotion;
    auto promo_pos = s.find('=');
    std::string move_part = s.substr(i);
    if (promo_pos != std::string::npos) {
        char p = s[promo_pos + 1];
        switch (toupper(p)) {
            case 'N': promotion = PieceType::knight; break;
            case 'B': promotion = PieceType::bishop; break;
            case 'R': promotion = PieceType::rook;   break;
            case 'Q': promotion = PieceType::queen;  break;
        }
        move_part = s.substr(i, promo_pos - i);
    }

    // Remove 'x' and '-'
    std::erase(move_part, 'x');
    std::erase(move_part, '-');

    // Last two chars are destination square
    if (move_part.size() < 2) throw std::invalid_argument("invalid san: " + san_str);
    std::string dest_str = move_part.substr(move_part.size() - 2);
    std::string disambig = move_part.substr(0, move_part.size() - 2);
    Square to_sq = parse_square(dest_str.c_str());

    // Disambiguation
    int from_file = -1, from_rank = -1;
    for (char c : disambig) {
        if (c >= 'a' && c <= 'h') from_file = c - 'a';
        if (c >= '1' && c <= '8') from_rank = c - '1';
    }

    Bitboard from_mask = pieces_mask(piece_type, turn);
    if (from_file >= 0) from_mask &= BB_FILES[from_file];
    if (from_rank >= 0) from_mask &= BB_RANKS[from_rank];
    Bitboard to_mask = BB_SQUARES(to_sq) & ~occupied_co[intOf(turn)];

    std::optional<Move> matched;
    for (auto& m : generate_legal_moves(from_mask, to_mask)) {
        if (m.promotion != promotion) continue;
        if (matched) throw std::invalid_argument("ambiguous san: " + san_str);
        matched = m;
    }
    if (!matched) throw std::invalid_argument("illegal san: " + san_str);
    return *matched;
}

inline Move Board::push_san(const std::string& san_str) {
    Move m = parse_san(san_str);
    push(m);
    return m;
}

// ─────────────────────────────────────────────
// UCI
// ─────────────────────────────────────────────

static std::string square_name(Square sq) {
    std::string s;
    s += (char)('a' + (intOf(sq) % 8));
    s += (char)('1' + (intOf(sq) / 8));
    return s;
}

inline std::string Board::uci(const Move& move, std::optional<bool> chess960_opt) const {
    bool c960 = chess960_opt.value_or(false);
    Move m = _to_chess960(move);
    m = _from_chess960(c960, m.from, m.to, m.promotion, m.drop);
    std::string s = square_name(m.from) + square_name(m.to);
    if (m.promotion) s += (char)(tolower(piece_char(*m.promotion)[0]));
    return s;
}

inline Move Board::parse_uci(const std::string& uci_str) {
    if (uci_str.size() < 4) throw std::invalid_argument("invalid uci: " + uci_str);

    Square from_sq = parse_square(uci_str.substr(0, 2).c_str());
    Square to_sq   = parse_square(uci_str.substr(2, 2).c_str());
    std::optional<PieceType> promo;
    if (uci_str.size() > 4) {
        switch (tolower(uci_str[4])) {
            case 'n': promo = PieceType::knight; break;
            case 'b': promo = PieceType::bishop; break;
            case 'r': promo = PieceType::rook;   break;
            case 'q': promo = PieceType::queen;  break;
        }
    }

    Move raw = promo ? Move{from_sq, to_sq, *promo} : Move{from_sq, to_sq};

    // Convert to chess960 notation and back to standard.
    Move move960 = _to_chess960(raw);
    Move move    = _from_chess960(false, move960.from, move960.to,
                                  move960.promotion, move960.drop);

    if (!is_legal(move)) throw std::invalid_argument("illegal uci: " + uci_str);
    return move;
}

inline Move Board::push_uci(const std::string& uci_str) {
    Move m = parse_uci(uci_str);
    push(m);
    return m;
}

// ─────────────────────────────────────────────
// Move classification
// ─────────────────────────────────────────────

inline bool Board::is_en_passant(const Move& move) const {
    return ep_square && *ep_square == move.to
        && (pawns & BB_SQUARES(move.from))
        && (abs(intOf(move.to) - intOf(move.from)) == 7
            || abs(intOf(move.to) - intOf(move.from)) == 9)
        && !(occupied & BB_SQUARES(move.to));
}

inline bool Board::is_capture(const Move& move) const {
    Bitboard touched = BB_SQUARES(move.from) ^ BB_SQUARES(move.to);
    return (bool)(touched & occupied_co[intOf(~turn)]) || is_en_passant(move);
}

inline bool Board::is_zeroing(const Move& move) const {
    Bitboard touched = BB_SQUARES(move.from) ^ BB_SQUARES(move.to);
    return (bool)(touched & pawns)
        || (bool)(touched & occupied_co[intOf(~turn)])
        || (move.drop && *move.drop == PieceType::pawn);
}

inline bool Board::_reduces_castling_rights(const Move& move) const {
    Bitboard cr = clean_castling_rights();
    Bitboard touched = BB_SQUARES(move.from) ^ BB_SQUARES(move.to);
    return (bool)(touched & cr)
        || (bool)(cr & BB_RANK_1 && touched & kings & occupied_co[intOf(Color::white)] & ~promoted)
        || (bool)(cr & BB_RANK_8 && touched & kings & occupied_co[intOf(Color::black)] & ~promoted);
}

inline bool Board::is_irreversible(const Move& move) const {
    return is_zeroing(move) || _reduces_castling_rights(move) || has_legal_en_passant();
}

inline bool Board::is_castling(const Move& move) const {
    if (kings & BB_SQUARES(move.from)) {
        int diff = (int)(file(move.from)) - (int)(file(move.to));
        return abs(diff) > 1 || (bool)(rooks & occupied_co[intOf(turn)] & BB_SQUARES(move.to));
    }
    return false;
}

inline bool Board::is_kingside_castling(const Move& move) const {
    return is_castling(move) && file(move.to) > file(move.from);
}

inline bool Board::is_queenside_castling(const Move& move) const {
    return is_castling(move) && file(move.to) < file(move.from);
}

// ─────────────────────────────────────────────
// Castling rights
// ─────────────────────────────────────────────

inline Bitboard Board::clean_castling_rights() const {
    if (!_stack.empty()) {
        // During a game push() keeps castling_rights filtered; return as-is.
        return castling_rights;
    }

    // At initial state or after set_fen: recompute from the board.
    Bitboard castling = castling_rights & rooks;
    Bitboard wc = castling & BB_RANK_1 & occupied_co[intOf(Color::white)];
    Bitboard bc = castling & BB_RANK_8 & occupied_co[intOf(Color::black)];

    // Standard chess: rooks must be on corner squares.
    wc &= (BB_A1 | BB_H1);
    bc &= (BB_A8 | BB_H8);

    // Remove castling rights if king is not on e1/e8.
    if (!(occupied_co[intOf(Color::white)] & kings & ~promoted & BB_E1)) wc = 0;
    if (!(occupied_co[intOf(Color::black)] & kings & ~promoted & BB_E8)) bc = 0;

    return wc | bc;
}

inline bool Board::has_castling_rights(Color color) const {
    Bitboard backrank = (color == Color::white) ? BB_RANK_1 : BB_RANK_8;
    return (bool)(clean_castling_rights() & backrank);
}

inline bool Board::has_kingside_castling_rights(Color color) const {
    Bitboard backrank = (color == Color::white) ? BB_RANK_1 : BB_RANK_8;
    Bitboard king_mask = kings & occupied_co[intOf(color)] & backrank & ~promoted;
    if (!king_mask) return false;
    Bitboard cr = clean_castling_rights() & backrank;
    while (cr) {
        Bitboard rook = cr & (~cr + 1); // lsb
        if (rook > king_mask) return true;
        cr &= cr - 1;
    }
    return false;
}

inline bool Board::has_queenside_castling_rights(Color color) const {
    Bitboard backrank = (color == Color::white) ? BB_RANK_1 : BB_RANK_8;
    Bitboard king_mask = kings & occupied_co[intOf(color)] & backrank & ~promoted;
    if (!king_mask) return false;
    Bitboard cr = clean_castling_rights() & backrank;
    while (cr) {
        Bitboard rook = cr & (~cr + 1); // lsb
        if (rook < king_mask) return true;
        cr &= cr - 1;
    }
    return false;
}

// ─────────────────────────────────────────────
// Status / validity
// ─────────────────────────────────────────────

inline status Board::get_status() const {
    uint32_t errors = 0;

    if (!occupied) errors |= (uint32_t)(status::EMPTY);
    if (!(occupied_co[intOf(Color::white)] & kings)) errors |= (uint32_t)(status::NO_WHITE_KING);
    if (!(occupied_co[intOf(Color::black)] & kings)) errors |= (uint32_t)(status::NO_BLACK_KING);
    if (std::popcount(occupied & kings) > 2)         errors |= (uint32_t)(status::TOO_MANY_KINGS);
    if (std::popcount(occupied_co[intOf(Color::white)]) > 16) errors |= (uint32_t)(status::TOO_MANY_WHITE_PIECES);
    if (std::popcount(occupied_co[intOf(Color::black)]) > 16) errors |= (uint32_t)(status::TOO_MANY_BLACK_PIECES);
    if (std::popcount(occupied_co[intOf(Color::white)] & pawns) > 8) errors |= (uint32_t)(status::TOO_MANY_WHITE_PAWNS);
    if (std::popcount(occupied_co[intOf(Color::black)] & pawns) > 8) errors |= (uint32_t)(status::TOO_MANY_BLACK_PAWNS);
    if (pawns & BB_BACKRANKS) errors |= (uint32_t)(status::PAWNS_ON_BACKRANK);
    if (castling_rights != clean_castling_rights()) errors |= (uint32_t)(status::BAD_CASTLING_RIGHTS);
    if (ep_square != _valid_ep_square()) errors |= (uint32_t)(status::INVALID_EP_SQUARE);
    if (was_into_check()) errors |= (uint32_t)(status::OPPOSITE_CHECK);

    Bitboard checkers_bb = checkers_mask();
    if (std::popcount(checkers_bb) > 2) errors |= (uint32_t)(status::TOO_MANY_CHECKERS);

    return (status)(errors);
}

inline bool Board::is_valid() const { return get_status() == status::VALID; }

// ─────────────────────────────────────────────
// EP skewer / slider blockers / safe
// ─────────────────────────────────────────────

inline std::optional<Square> Board::_valid_ep_square() const {
    if (!ep_square) return std::nullopt;
    int ep_rank = (turn == Color::white) ? 5 : 2;
    if (rank(*ep_square) != ep_rank) return std::nullopt;

    Bitboard pawn_mask = (turn == Color::white)
        ? BB_SQUARES(*ep_square) >> 8
        : BB_SQUARES(*ep_square) << 8;

    if (!(pawns & occupied_co[intOf(~turn)] & pawn_mask)) return std::nullopt;
    if (occupied & BB_SQUARES(*ep_square)) return std::nullopt;

    Bitboard seventh = (turn == Color::white)
        ? BB_SQUARES(*ep_square) << 8
        : BB_SQUARES(*ep_square) >> 8;
    if (occupied & seventh) return std::nullopt;

    return ep_square;
}

inline bool Board::_ep_skewered(Square king_sq, Square capturer) const {
    assert(ep_square.has_value());
    int down = (turn == Color::white) ? -8 : 8;
    Square last_double = (Square)(intOf(*ep_square) + down);

    Bitboard occ = (occupied & ~BB_SQUARES(last_double) & ~BB_SQUARES(capturer))
                   | BB_SQUARES(*ep_square);

    Bitboard h_attackers = occupied_co[intOf(~turn)] & (rooks | queens);
    if (BB_RANK_ATTACKS[intOf(king_sq)].at(BB_RANK_MASKS[intOf(king_sq)] & occ) & h_attackers)
        return true;

    Bitboard d_attackers = occupied_co[intOf(~turn)] & (bishops | queens);
    if (BB_DIAG_ATTACKS[intOf(king_sq)].at(BB_DIAG_MASKS[intOf(king_sq)] & occ) & d_attackers)
        return true;

    return false;
}

inline Bitboard Board::_slider_blockers(Square king_sq) const {
    Bitboard rq = rooks | queens;
    Bitboard bq = bishops | queens;

    Bitboard snipers =
        (BB_RANK_ATTACKS[intOf(king_sq)].at(0) & rq) |
        (BB_FILE_ATTACKS[intOf(king_sq)].at(0) & rq) |
        (BB_DIAG_ATTACKS[intOf(king_sq)].at(0) & bq);

    Bitboard blockers = 0;
    for (Square sniper : scan_reversed(snipers & occupied_co[intOf(~turn)])) {
        Bitboard b = between(king_sq, sniper) & occupied;
        if (b && (BB_SQUARES((Square)(msb(b))) == b))
            blockers |= b;
    }
    return blockers & occupied_co[intOf(turn)];
}

// ─────────────────────────────────────────────
// BUG FIX: _is_safe - king move x-ray attack detection
// ─────────────────────────────────────────────
// Previously used is_attacked_by(~turn, move.to) which checks attacks using
// the current `occupied` bitboard. This still has the king at move.from, so
// sliding pieces (queen/rook/bishop) are blocked by the king's current square,
// making the king's destination appear safe even when it is attacked through
// the king's vacated square.
//
// Fix: call _attackers_mask with occupied ^ BB_SQUARES(king_sq) so that the
// king is removed from the occupancy, allowing sliding attacks to be detected
// correctly through the square the king is leaving.
// ─────────────────────────────────────────────

inline bool Board::_is_safe(Square king_sq, Bitboard blockers, const Move& move) const {
    if (move.from == king_sq) {
        if (is_castling(move)) return true;
        // Remove king from occupancy to detect x-ray attacks through the vacated square.
        return !(_attackers_mask(~turn, move.to, occupied ^ BB_SQUARES(king_sq)));
    }
    if (is_en_passant(move)) {
        return (bool)(pin_mask(turn, move.from) & BB_SQUARES(move.to))
            && !_ep_skewered(king_sq, move.from);
    }
    return !(blockers & BB_SQUARES(move.from))
        || (bool)(ray(move.from, move.to) & BB_SQUARES(king_sq));
}

// ─────────────────────────────────────────────
// Evasions
// ─────────────────────────────────────────────

inline vector<Move> Board::_generate_evasions(Square king_sq, Bitboard checkers_bb,
                                              Bitboard from_mask, Bitboard to_mask) const {
    vector<Move> moves;
    Bitboard sliders = checkers_bb & (bishops | rooks | queens);

    Bitboard attacked = 0;
    for (Square checker : scan_reversed(sliders))
        attacked |= ray(king_sq, checker) & ~BB_SQUARES(checker);

    if (BB_SQUARES(king_sq) & from_mask) {
        for (Square to_sq : scan_reversed(
                BB_KING_ATTACKS[intOf(king_sq)]
                & ~occupied_co[intOf(turn)] & ~attacked & to_mask))
            moves.emplace_back(king_sq, to_sq);
    }

    Square checker_sq = (Square)(msb(checkers_bb));
    if (BB_SQUARES(checker_sq) == checkers_bb) {
        Bitboard target = between(king_sq, checker_sq) | checkers_bb;
        auto pseudo = generate_pseudo_legal_moves(~kings & from_mask, target & to_mask);
        moves.insert(moves.end(), pseudo.begin(), pseudo.end());

        if (ep_square && !(BB_SQUARES(*ep_square) & target)) {
            int down = (turn == Color::white) ? -8 : 8;
            Square last_double = (Square)(intOf(*ep_square) + down);
            if (last_double == checker_sq) {
                auto ep = generate_pseudo_legal_ep(from_mask, to_mask);
                moves.insert(moves.end(), ep.begin(), ep.end());
            }
        }
    }
    return moves;
}

// ─────────────────────────────────────────────
// Legal move generation
// ─────────────────────────────────────────────

inline bool Board::_attacked_for_king(Bitboard path, Bitboard occ) const {
    for (Square sq : scan_reversed(path))
        if (_attackers_mask(~turn, sq, occ)) return true;
    return false;
}

inline vector<Move> Board::generate_legal_moves(Bitboard from_mask, Bitboard to_mask) const {
    if (is_variant_end()) return {};

    vector<Move> moves;
    Bitboard king_mask = kings & occupied_co[intOf(turn)];

    if (king_mask) {
        Square king_sq = (Square)(msb(king_mask));
        Bitboard blockers  = _slider_blockers(king_sq);
        Bitboard checkers_bb = attackers_mask(~turn, king_sq);

        if (checkers_bb) {
            for (auto& m : _generate_evasions(king_sq, checkers_bb, from_mask, to_mask))
                if (_is_safe(king_sq, blockers, m)) moves.push_back(m);
        } else {
            for (auto& m : generate_pseudo_legal_moves(from_mask, to_mask))
                if (_is_safe(king_sq, blockers, m)) moves.push_back(m);
        }
    } else {
        moves = generate_pseudo_legal_moves(from_mask, to_mask);
    }
    return moves;
}

inline vector<Move> Board::generate_legal_ep(Bitboard from_mask, Bitboard to_mask) const {
    if (is_variant_end()) return {};
    vector<Move> moves;
    for (auto& m : generate_pseudo_legal_ep(from_mask, to_mask))
        if (!is_into_check(m)) moves.push_back(m);
    return moves;
}

inline vector<Move> Board::generate_legal_captures(Bitboard from_mask, Bitboard to_mask) const {
    auto a = generate_legal_moves(from_mask, to_mask & occupied_co[intOf(~turn)]);
    auto b = generate_legal_ep(from_mask, to_mask);
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

// ─────────────────────────────────────────────
// Castling move generation
// ─────────────────────────────────────────────

inline vector<Move> Board::generate_castling_moves(Bitboard from_mask, Bitboard to_mask) const {
    if (is_variant_end()) return {};

    vector<Move> moves;
    Bitboard backrank = (turn == Color::white) ? BB_RANK_1 : BB_RANK_8;
    Bitboard king_bb = occupied_co[intOf(turn)] & kings & ~promoted & backrank & from_mask;
    king_bb &= (~king_bb + 1); // lsb
    if (!king_bb) return moves;

    Bitboard bb_c = BB_FILE_C & backrank;
    Bitboard bb_d = BB_FILE_D & backrank;
    Bitboard bb_f = BB_FILE_F & backrank;
    Bitboard bb_g = BB_FILE_G & backrank;

    for (Square candidate : scan_reversed(clean_castling_rights() & backrank & to_mask)) {
        Bitboard rook_bb = BB_SQUARES(candidate);
        bool a_side = rook_bb < king_bb;
        Bitboard king_to = a_side ? bb_c : bb_g;
        Bitboard rook_to = a_side ? bb_d : bb_f;

        Square king_sq    = (Square)(msb(king_bb));
        Square king_to_sq = (Square)(msb(king_to));
        Square rook_to_sq = (Square)(msb(rook_to));

        Bitboard king_path = between(king_sq, king_to_sq);
        Bitboard rook_path = between(candidate, rook_to_sq);

        if (!((occupied ^ king_bb ^ rook_bb) & (king_path | rook_path | king_to | rook_to))
            && !_attacked_for_king(king_path | king_bb, occupied ^ king_bb)
            && !_attacked_for_king(king_to, occupied ^ king_bb ^ rook_bb ^ rook_to))
        {
            moves.push_back(_from_chess960(false, king_sq, candidate));
        }
    }
    return moves;
}

// ─────────────────────────────────────────────
// Chess960 helpers
// ─────────────────────────────────────────────

inline Move Board::_from_chess960(bool c960, Square from_sq, Square to_sq,
                                  std::optional<PieceType> promo,
                                  std::optional<PieceType> drop) const {
    if (!c960 && !promo && !drop) {
        if (from_sq == Square::E1 && (kings & BB_E1)) {
            if (to_sq == Square::H1) return Move{Square::E1, Square::G1};
            if (to_sq == Square::A1) return Move{Square::E1, Square::C1};
        } else if (from_sq == Square::E8 && (kings & BB_E8)) {
            if (to_sq == Square::H8) return Move{Square::E8, Square::G8};
            if (to_sq == Square::A8) return Move{Square::E8, Square::C8};
        }
    }
    if (promo) return Move{from_sq, to_sq, *promo};
    return Move{from_sq, to_sq};
}

inline Move Board::_to_chess960(const Move& move) const {
    if (move.from == Square::E1 && (kings & BB_E1)) {
        if (move.to == Square::G1 && !(rooks & BB_G1)) return Move{Square::E1, Square::H1};
        if (move.to == Square::C1 && !(rooks & BB_C1)) return Move{Square::E1, Square::A1};
    } else if (move.from == Square::E8 && (kings & BB_E8)) {
        if (move.to == Square::G8 && !(rooks & BB_G8)) return Move{Square::E8, Square::H8};
        if (move.to == Square::C8 && !(rooks & BB_C8)) return Move{Square::E8, Square::A8};
    }
    return move;
}

// ─────────────────────────────────────────────
// Transposition key
// ─────────────────────────────────────────────

inline std::tuple<Bitboard,Bitboard,Bitboard,Bitboard,Bitboard,Bitboard,
           Bitboard,Bitboard,Color,Bitboard,std::optional<Square>>
Board::_transposition_key() const {
    return {pawns, knights, bishops, rooks, queens, kings,
            occupied_co[0], occupied_co[1], turn,
            clean_castling_rights(),
            has_legal_en_passant() ? ep_square : std::nullopt};
}

// ─────────────────────────────────────────────
// Equality
// ─────────────────────────────────────────────

inline bool Board::operator==(const Board& other) const {
    return halfmove_clock == other.halfmove_clock
        && fullmove_number == other.fullmove_number
        && _transposition_key() == other._transposition_key();
}

// ─────────────────────────────────────────────
// Transform / mirror / copy
// ─────────────────────────────────────────────

inline void Board::apply_transform(Bitboard (*f)(Bitboard)) {
    BaseBoard::apply_transform(f);
    clear_stack();
    ep_square = ep_square
        ? std::optional<Square>((Square)(msb(f(BB_SQUARES(*ep_square)))))
        : std::nullopt;
    castling_rights = f(castling_rights);
}

inline Board Board::transform(Bitboard (*f)(Bitboard)) const {
    Board b = copy(false);
    b.apply_transform(f);
    return b;
}

inline void Board::apply_mirror() {
    BaseBoard::apply_mirror();
    turn = ~turn;
}

inline Board Board::mirror() const {
    Board b = copy();
    b.apply_mirror();
    return b;
}

inline Board Board::copy(bool stack) const {
    Board b(std::nullopt);
    b.pawns   = pawns;   b.knights = knights; b.bishops = bishops;
    b.rooks   = rooks;   b.queens  = queens;  b.kings   = kings;
    b.promoted = promoted;
    b.occupied_co = occupied_co;
    b.occupied = occupied;
    b.turn = turn; b.castling_rights = castling_rights;
    b.ep_square = ep_square; b.halfmove_clock = halfmove_clock;
    b.fullmove_number = fullmove_number;
    if (stack) {
        b.move_stack = move_stack;
        b._stack = _stack;
    }
    return b;
}

inline Board Board::empty() {
    return {std::nullopt};
}

inline std::ostream& operator<<(std::ostream& os, const Board& board) {
    for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
            Square sq = (Square)(r * 8 + f);
            auto pt_col = board.piece_at(sq);
            char c = '.';
            if (pt_col) {
                auto [pt, col] = *pt_col;
                switch (pt) {
                    case PieceType::pawn:   c = 'p'; break;
                    case PieceType::knight: c = 'n'; break;
                    case PieceType::bishop: c = 'b'; break;
                    case PieceType::rook:   c = 'r'; break;
                    case PieceType::queen:  c = 'q'; break;
                    case PieceType::king:   c = 'k'; break;
                }
                if (col == Color::white) c = toupper(c);
            }
            os << c << ' ';
        }
        os << '\n';
    }
    return os;
}

NAMESPACE_END