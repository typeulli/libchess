/*
Derived from python-chess
Copyright (C) 2012-2021 Niklas Fiekas

Translated to C++ by typeulli (2026)
Licensed under GPL v3
*/

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include "libchess.hpp"
#include "board.ipp"


NAMESPACE_BEGIN
std::string to_string(Color color) {
    return color == Color::white ? "white" : "black";
}

Color operator~(Color color) {
    return color == Color::white ? Color::black : Color::white;
}

int intOf(Color color) {
    return (int)(color);
}
std::string to_string(Termination termination) {
    switch (termination) {
        case Termination::CHECKMATE: return "checkmate";
        case Termination::STALEMATE: return "stalemate";
        case Termination::INSUFFICIENT_MATERIAL: return "insufficient_material";
        case Termination::SEVENTYFIVE_MOVES: return "seventyfive_moves";
        case Termination::FIFTY_MOVES: return "fifty_moves";
        case Termination::THREEFOLD_REPETITION: return "threefold_repetition";
        case Termination::FIVEFOLD_REPETITION: return "fivefold_repetition";
        case Termination::VARIANT_WIN: return "variant_win";
        case Termination::VARIANT_LOSS: return "variant_loss";
        case Termination::VARIANT_DRAW: return "variant_draw";
    }
    return "";
}
std::string to_string(Outcome outcome) {
    return "R[" + to_string(outcome.termination) + ":" + to_string(outcome.winner) + "]";
}
std::string to_string(Square square) {
    char file = 'a' + (int)(square) % 8;
    char rank = '1' + (int)(square) / 8;
    return std::string() + file + rank;
}
int intOf(Square square) {
    return (int)(square);
}
std::string to_string(PieceType piece_type) {
    switch (piece_type) {
        case PieceType::pawn:   return "pawn";
        case PieceType::knight: return "knight";
        case PieceType::bishop: return "bishop";
        case PieceType::rook:   return "rook";
        case PieceType::queen:  return "queen";
        case PieceType::king:   return "king";
        case PieceType::none:   return "none";
    }
    return "";
}

int intOf(PieceType piece_type) {
    return (int)(piece_type);
}

Square& operator++(Square& square) {
    square = (Square)((uint8_t)(square) + 1);
    return square;
}

Square parse_square(const char *str) {
    return (Square)((str[0] - 'a') + 8 * (str[1] - '1'));
}

Square at(int file, int rank) {
    return (Square)(file + 8 * rank);
}
uint8_t file(Square square) {
    return (uint8_t)(square) % 8;
}
uint8_t rank(Square square) {
    return (uint8_t)(square) / 8;
}

uint8_t square_distance(Square a, Square b) {
    uint8_t file_distance = std::abs(file(a) - file(b));
    uint8_t rank_distance = std::abs(rank(a) - rank(b));
    return std::max(file_distance, rank_distance);
}

uint8_t square_manhattan_distance(Square a, Square b) {
    uint8_t file_distance = std::abs(file(a) - file(b));
    uint8_t rank_distance = std::abs(rank(a) - rank(b));
    return file_distance + rank_distance;
}

uint8_t square_knight_distance(Square a, Square b) {
    uint8_t dx = std::abs(file(a) - file(b));
    uint8_t dy = std::abs(rank(a) - rank(b));
    uint8_t sum = dx + dy;
    if (sum == 1) {
        return 3; // One move away, but not a knight move
    }
    if (dx == 2 && dy == 2) {
        return 4; // Two moves away, but not a knight move
    }
    if (dx == 1 && dy == 1) {
        if (BB_SQUARES(a) & BB_CORNERS | BB_SQUARES(b) & BB_CORNERS) {
            return 4; // Two moves away, but not a knight move
        }
    }
    double d = std::max(std::max(dx,dy) / 2.0, sum / 3.0);
    auto m = (uint8_t)(std::ceil(d));
    return m + ((m + sum) & 1);
}

Square mirror(Square square) {
    return (Square)((uint8_t)(square) ^ 0x38);
}

const Bitboard BB_FILES[8] = {BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F, BB_FILE_G, BB_FILE_H};
const Bitboard BB_RANKS[8] = {BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8};

inline int lsb(Bitboard bb) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return (int)(idx);
#else
    return __builtin_ctzll(bb);
#endif
}

scan_forward::scan_forward(Bitboard b) : bb(b) {}
Square scan_forward::iterator::operator*() const { return (Square)(lsb(bb)); }
scan_forward::iterator& scan_forward::iterator::operator++() { bb &= bb - 1; return *this; }
bool scan_forward::iterator::operator!=(const iterator& other) const { return bb != other.bb; }
scan_forward::iterator scan_forward::begin() const { return {bb}; }
scan_forward::iterator scan_forward::end() { return {0}; }

inline int msb(Bitboard bb) {
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse64(&idx, bb);
    return (int)(idx);
#else
    return 63 - __builtin_clzll(bb);
#endif
}
scan_reversed::scan_reversed(Bitboard b) : bb(b) {}
Square scan_reversed::iterator::operator*() const { return (Square)(msb(bb)); }
scan_reversed::iterator& scan_reversed::iterator::operator++() { bb ^= (1ULL << (uint8_t)(operator*())); return *this; }
bool scan_reversed::iterator::operator!=(const iterator& other) const { return bb != other.bb; }
scan_reversed::iterator scan_reversed::begin() const { return {bb}; }
scan_reversed::iterator scan_reversed::end() { return {0}; }


inline Bitboard flip_vertical(Bitboard bb) {
    bb = ((bb >> 8) & 0x00ff00ff00ff00ffULL) | ((bb & 0x00ff00ff00ff00ffULL) << 8);
    bb = ((bb >> 16) & 0x0000ffff0000ffffULL) | ((bb & 0x0000ffff0000ffffULL) << 16);
    bb = (bb >> 32) | ((bb & 0x00000000ffffffffULL) << 32);
    return bb;
}

inline Bitboard flip_horizontal(Bitboard bb) {
    bb = ((bb >> 1) & 0x5555555555555555ULL) | ((bb & 0x5555555555555555ULL) << 1);
    bb = ((bb >> 2) & 0x3333333333333333ULL) | ((bb & 0x3333333333333333ULL) << 2);
    bb = ((bb >> 4) & 0x0f0f0f0f0f0f0f0fULL) | ((bb & 0x0f0f0f0f0f0f0f0fULL) << 4);
    return bb;
}

inline Bitboard flip_diagonal(Bitboard bb) {
    Bitboard t = (bb ^ (bb << 28)) & 0x0f0f0f0f00000000ULL;
    bb = bb ^ t ^ (t >> 28);
    t = (bb ^ (bb << 14)) & 0x3333000033330000ULL;
    bb = bb ^ t ^ (t >> 14);
    t = (bb ^ (bb << 7)) & 0x5500550055005500ULL;
    bb = bb ^ t ^ (t >> 7);
    return bb;
}

inline Bitboard flip_anti_diagonal(Bitboard bb) {
    Bitboard t = bb ^ (bb << 36);
    bb = bb ^ ((t ^ (bb >> 36)) & 0xf0f0f0f00f0f0f0fULL);
    t = (bb ^ (bb << 18)) & 0xcccc0000cccc0000ULL;
    bb = bb ^ t ^ (t >> 18);
    t = (bb ^ (bb << 9)) & 0xaa00aa00aa00aa00ULL;
    bb = bb ^ t ^ (t >> 9);
    return bb;
}

inline Bitboard shift_down(Bitboard bb) { return bb >> 8; }
inline Bitboard shift_2_down(Bitboard bb) { return bb >> 16; }
inline Bitboard shift_up(Bitboard bb) { return (bb << 8) & BB_ALL; }
inline Bitboard shift_2_up(Bitboard bb) { return (bb << 16) & BB_ALL; }
inline Bitboard shift_right(Bitboard bb) { return (bb << 1) & ~BB_FILE_A & BB_ALL; }
inline Bitboard shift_2_right(Bitboard bb) { return (bb << 2) & ~BB_FILE_A & ~BB_FILE_B & BB_ALL; }
inline Bitboard shift_left(Bitboard bb) { return (bb >> 1) & ~BB_FILE_H; }
inline Bitboard shift_2_left(Bitboard bb) { return (bb >> 2) & ~BB_FILE_G & ~BB_FILE_H; }
inline Bitboard shift_up_left(Bitboard bb) { return (bb << 7) & ~BB_FILE_H & BB_ALL; }
inline Bitboard shift_up_right(Bitboard bb) { return (bb << 9) & ~BB_FILE_A & BB_ALL; }
inline Bitboard shift_down_left(Bitboard bb) { return (bb >> 9) & ~BB_FILE_H; }
inline Bitboard shift_down_right(Bitboard bb) { return (bb >> 7) & ~BB_FILE_A; }

auto operator<=>(Square lhs, int rhs) {
    return (int)(lhs) <=> rhs;
}

Bitboard _sliding_attacks(Square square, Bitboard occupied, const std::vector<int>& deltas) {
    Bitboard attacks = 0;
    for (int delta : deltas) {
        Square s = square;
        while (true) {
            s = (Square)((int)(s) + delta);
            if (s < 0 || s >= 64) break;
            if (square_distance(s, (Square)((int)(s) - delta)) > 2) break;
            attacks |= BB_SQUARES(s);
            if (BB_SQUARES(s) & occupied) break;
        }
    }
    return attacks;
}

Bitboard _step_attacks(Square square, const std::vector<int>& deltas) {
    return _sliding_attacks(square, BB_ALL, deltas);
}
const std::array<Bitboard, 64> BB_KNIGHT_ATTACKS = []() {
    std::array<Bitboard, 64> table{};
    for (Square square = Square::A1; square <= Square::H8; ++square) {
        table[(int)(square)] = _step_attacks(square, {15, 17, 10, 6, -15, -17, -10, -6});
    }
    return table;
}();
const std::array<Bitboard, 64> BB_KING_ATTACKS = []() {
    std::array<Bitboard, 64> table{};
    for (Square square = Square::A1; square <= Square::H8; ++square) {
        table[(int)(square)] = _step_attacks(square, {9, 8, 7, 1, -9, -8, -7, -1});
    }
    return table;
}();
const std::array<std::array<Bitboard, 64>, 2> BB_PAWN_ATTACKS = []() {
    std::array<std::array<Bitboard, 64>, 2> table{};
    for (Square square = Square::A1; square <= Square::H8; ++square) {
        table[intOf(Color::white)][(int)(square)] = _step_attacks(square, {7, 9});
        table[intOf(Color::black)][(int)(square)] = _step_attacks(square, {-7, -9});
    }
    return table;
}();

Bitboard _carry_rippler::iterator::operator*() const { return subset; }

_carry_rippler::iterator& _carry_rippler::iterator::operator++() {
    subset = (subset - mask) & mask;
    if (subset == 0) done = true;
    return *this;
}

bool _carry_rippler::iterator::operator!=(const _carry_rippler::iterator& other) const {
    return done != other.done;
}
_carry_rippler::_carry_rippler(Bitboard m) : mask(m) {}
_carry_rippler::iterator _carry_rippler::begin() const {
    return {mask, 0, false};
}
_carry_rippler::iterator _carry_rippler::end() {
    return {0, 0, true};
}

Bitboard _edges(Square square) {
    uint8_t sq_file = file(square);
    uint8_t sq_rank = rank(square);
    return (((BB_RANK_1 | BB_RANK_8) & ~BB_RANKS[sq_rank]) |
        ((BB_FILE_A | BB_FILE_H) & ~BB_FILES[sq_file]));
}


std::pair<std::vector<Bitboard>, std::vector<std::map<Bitboard, Bitboard>>> _attack_table(const std::vector<int>& deltas) {
    std::vector<Bitboard> mask_table;
    std::vector<std::map<Bitboard, Bitboard>> attack_table;

    for (Square square = Square::A1; square < 64; ++square) {
        std::map<Bitboard, Bitboard> attacks;

        Bitboard mask = _sliding_attacks(square, 0, deltas) & ~_edges(square);
        for (Bitboard subset : _carry_rippler(mask)) {
            attacks[subset] = _sliding_attacks(square, subset, deltas);
        }
        
        attack_table.push_back(attacks);
        mask_table.push_back(mask);
    }

    return {mask_table, attack_table};
}

const std::vector<Bitboard> BB_DIAG_MASKS = _attack_table({-9, -7, 7, 9}).first;
const std::vector<Bitboard> BB_FILE_MASKS = _attack_table({-8, 8}).first;
const std::vector<Bitboard> BB_RANK_MASKS = _attack_table({-1, 1}).first;

const std::vector<std::map<Bitboard, Bitboard>> BB_DIAG_ATTACKS = _attack_table({-9, -7, 7, 9}).second;
const std::vector<std::map<Bitboard, Bitboard>> BB_FILE_ATTACKS = _attack_table({-8, 8}).second;
const std::vector<std::map<Bitboard, Bitboard>> BB_RANK_ATTACKS = _attack_table({-1, 1}).second;

std::vector<std::vector<Bitboard>> _rays() {
    std::vector<std::vector<Bitboard>> rays(64, std::vector<Bitboard>(64, 0));

    for (int a = 0; a < 64; ++a) {
        int ar = a / 8;
        int af = a % 8;

        for (int b = 0; b < 64; ++b) {
            int br = b / 8;
            int bf = b % 8;

            int dr = br - ar;
            int df = bf - af;

            int step = 0;

            if (dr == 0 && df != 0) step = (df > 0 ? 1 : -1);          // rank
            else if (df == 0 && dr != 0) step = (dr > 0 ? 8 : -8);     // file
            else if (dr == df && dr != 0) step = (dr > 0 ? 9 : -9);    // diag \
            else if (dr == -df && dr != 0) step = (dr > 0 ? 7 : -7);   // diag /
            else continue;

            Bitboard ray = BB_SQUARES((Square)(a));

            int sq = a + step;
            while (sq >= 0 && sq < 64) {
                ray |= BB_SQUARES((Square)(sq));

                if (sq == b) break;

                int r = sq / 8;
                int f = sq % 8;

                if (std::abs(r - ar) != std::abs((sq - a) / step) &&
                    step != 1 && step != -1 && step != 8 && step != -8)
                    break;

                sq += step;
            }

            rays[a][b] = ray;
        }
    }

    return rays;
}

const std::vector<std::vector<Bitboard>> BB_RAYS = _rays();

Bitboard ray(Square from, Square to) {
    return BB_RAYS[(int)(from)][(int)(to)];
}
Bitboard between(Square from, Square to) {
    Bitboard bb = BB_RAYS[(int)(from)][(int)(to)] & ((BB_ALL << (int)(from)) ^ (BB_ALL << (int)(to)));
    return bb & (bb - 1);
}

Move Move::from_uci(const std::string &uci_str) {
    if (uci_str.length() < 4) throw std::invalid_argument("UCI string must be at least 4 characters long");
    Square from = parse_square(uci_str.substr(0, 2).c_str());
    Square to = parse_square(uci_str.substr(2, 2).c_str());

    if (uci_str.length() > 4) {
        PieceType promotion;
        char promo_char = uci_str[4];
        switch (promo_char) {
            case 'q': promotion = PieceType::queen; break;
            case 'r': promotion = PieceType::rook; break;
            case 'b': promotion = PieceType::bishop; break;
            case 'n': promotion = PieceType::knight; break;
            default: throw std::invalid_argument("Invalid promotion piece in UCI string");
        }
        return {from, to, promotion};
    }
    return {from, to};
}

std::string Move::uci() const {
    std::string str = to_string(from) + to_string(to);
    if (promotion.has_value()) {
        char promo_char;
        switch (promotion.value()) {
            case PieceType::queen:  promo_char = 'q'; break;
            case PieceType::rook:   promo_char = 'r'; break;
            case PieceType::bishop: promo_char = 'b'; break;
            case PieceType::knight: promo_char = 'n'; break;
            default: promo_char = '?';
        }
        str += promo_char;
    }
    return str;
}

inline std::string to_string(const Move &move) {
    return move.uci();
}

void BaseBoard::reset_board() { _reset_board(); }
void BaseBoard::clear_board() { _clear_board(); }

Bitboard BaseBoard::pieces_mask(PieceType piece_type, Color color) const {
    Bitboard bb;
    switch (piece_type) {
        case PieceType::pawn:   bb = pawns;   break;
        case PieceType::knight: bb = knights; break;
        case PieceType::bishop: bb = bishops; break;
        case PieceType::rook:   bb = rooks;   break;
        case PieceType::queen:  bb = queens;  break;
        case PieceType::king:   bb = kings;   break;
        default: assert(false && "expected PieceType"); bb = BB_EMPTY;
    }
    return bb & occupied_co[(uint8_t)(color)];
}

Bitboard BaseBoard::pieces(PieceType piece_type, Color color) const {
    return pieces_mask(piece_type, color);
}

std::optional<Piece> BaseBoard::piece_at(Square square) const {
    auto piece_type = piece_type_at(square);
    if (piece_type.has_value()) {
        Bitboard mask = BB_SQUARES(square);
        Color color = (occupied_co[(uint8_t)(Color::white)] & mask) ? Color::white : Color::black;
        return Piece{piece_type.value(), color};
    }
    return std::nullopt;
}

std::optional<PieceType> BaseBoard::piece_type_at(Square square) const {
    Bitboard mask = BB_SQUARES(square);
    if (!(occupied & mask)) return std::nullopt;
    if (pawns & mask)   return PieceType::pawn;
    if (knights & mask) return PieceType::knight;
    if (bishops & mask) return PieceType::bishop;
    if (rooks & mask)   return PieceType::rook;
    if (queens & mask)  return PieceType::queen;
    return PieceType::king;
}

std::optional<Color> BaseBoard::color_at(Square square) const {
    Bitboard mask = BB_SQUARES(square);
    if (occupied_co[(uint8_t)(Color::white)] & mask) return Color::white;
    if (occupied_co[(uint8_t)(Color::black)] & mask) return Color::black;
    return std::nullopt;
}

std::optional<Square> BaseBoard::king(Color color) const {
    Bitboard king_mask = occupied_co[(uint8_t)(color)] & kings & ~promoted;
    return king_mask ? std::optional<Square>((Square)(msb(king_mask))) : std::nullopt;
}
Bitboard BaseBoard::attacks_mask(Square square) const {
    Bitboard bb_square = BB_SQUARES(square);
    if (bb_square & pawns) {
        Color color = (bb_square & occupied_co[(uint8_t)(Color::white)]) ? Color::white : Color::black;
        return BB_PAWN_ATTACKS[(uint8_t)(color)][(int)(square)];
    } else if (bb_square & knights) {
        return BB_KNIGHT_ATTACKS[(int)(square)];
    } else if (bb_square & kings) {
        return BB_KING_ATTACKS[(int)(square)];
    } else {
        Bitboard attacks = 0;
        if (bb_square & bishops || bb_square & queens) {
            attacks = BB_DIAG_ATTACKS[(int)(square)].at(BB_DIAG_MASKS[(int)(square)] & occupied);
        }
        if (bb_square & rooks || bb_square & queens) {
            attacks |= BB_RANK_ATTACKS[(int)(square)].at(BB_RANK_MASKS[(int)(square)] & occupied) |
                       BB_FILE_ATTACKS[(int)(square)].at(BB_FILE_MASKS[(int)(square)] & occupied);
        }
        return attacks;
    }
}
Bitboard BaseBoard::_attackers_mask(Color color, Square square, Bitboard occ) const {
    Bitboard rank_pieces = BB_RANK_MASKS[(int)(square)] & occ;
    Bitboard file_pieces = BB_FILE_MASKS[(int)(square)] & occ;
    Bitboard diag_pieces = BB_DIAG_MASKS[(int)(square)] & occ;

    Bitboard queens_and_rooks = queens | rooks;
    Bitboard queens_and_bishops = queens | bishops;

    Bitboard attackers =
        (BB_KING_ATTACKS[intOf(square)] & kings) |
        (BB_KNIGHT_ATTACKS[intOf(square)] & knights) |
        (BB_RANK_ATTACKS[intOf(square)].at(rank_pieces) & queens_and_rooks) |
        (BB_FILE_ATTACKS[intOf(square)].at(file_pieces) & queens_and_rooks) |
        (BB_DIAG_ATTACKS[intOf(square)].at(diag_pieces) & queens_and_bishops) |
        (BB_PAWN_ATTACKS[intOf(~color)][intOf(square)] & pawns);

    return attackers & occupied_co[intOf(color)];
}

Bitboard BaseBoard::attackers_mask(Color color, Square square) const {
    return _attackers_mask(color, square, occupied);
}

Bitboard BaseBoard::attackers_mask(Color color, Square square, Bitboard occ) const {
    return _attackers_mask(color, square, occ);
}

bool BaseBoard::is_attacked_by(Color color, Square square) const {
    return attackers_mask(color, square) != 0;
}

Bitboard BaseBoard::pin_mask(Color color, Square square) const {
    auto king_sq = king(color);
    if (!king_sq.has_value()) return BB_ALL;

    Bitboard square_mask = BB_SQUARES(square);

    const auto checks = std::array{
        std::pair{ &BB_FILE_ATTACKS, rooks | queens },
        std::pair{ &BB_RANK_ATTACKS, rooks | queens },
        std::pair{ &BB_DIAG_ATTACKS, bishops | queens },
    };

    for (auto& [attacks_ptr, sliders] : checks) {
        const auto& attacks = *attacks_ptr;
        Bitboard rays = attacks[(int)(king_sq.value())].at(0);
        if (rays & square_mask) {
            Bitboard snipers = rays & sliders & occupied_co[!intOf(color)];
            for (Square sniper : scan_reversed(snipers)) {
                if ((between(sniper, king_sq.value()) & (occupied | square_mask)) == square_mask) {
                    return ray(king_sq.value(), sniper);
                }
            }
            break;
        }
    }
    return BB_ALL;
}

bool BaseBoard::is_pinned(Color color, Square square) const {
    return pin_mask(color, square) != BB_ALL;
}

std::optional<PieceType> BaseBoard::_remove_piece_at(Square square) {
    auto piece_type = piece_type_at(square);
    Bitboard mask = BB_SQUARES(square);

    switch (piece_type.value_or((PieceType)(0))) {
        case PieceType::pawn:   pawns   ^= mask; break;
        case PieceType::knight: knights ^= mask; break;
        case PieceType::bishop: bishops ^= mask; break;
        case PieceType::rook:   rooks   ^= mask; break;
        case PieceType::queen:  queens  ^= mask; break;
        case PieceType::king:   kings   ^= mask; break;
        default: return std::nullopt;
    }

    occupied ^= mask;
    occupied_co[intOf(Color::white)] &= ~mask;
    occupied_co[intOf(Color::black)] &= ~mask;
    promoted &= ~mask;

    return piece_type;
}

std::optional<Piece> BaseBoard::remove_piece_at(Square square) {
    Color color = (occupied_co[intOf(Color::white)] & BB_SQUARES(square)) ? Color::white : Color::black;
    auto piece_type = _remove_piece_at(square);
    if (piece_type.has_value()) return Piece{piece_type.value(), color};
    return std::nullopt;
}

void BaseBoard::_set_piece_at(Square square, PieceType piece_type, Color color, bool promoted_piece) {
    _remove_piece_at(square);
    Bitboard mask = BB_SQUARES(square);

    switch (piece_type) {
        case PieceType::pawn:   pawns   |= mask; break;
        case PieceType::knight: knights |= mask; break;
        case PieceType::bishop: bishops |= mask; break;
        case PieceType::rook:   rooks   |= mask; break;
        case PieceType::queen:  queens  |= mask; break;
        case PieceType::king:   kings   |= mask; break;
        default: return;
    }

    occupied ^= mask;
    occupied_co[intOf(color)] ^= mask;
    if (promoted_piece) promoted ^= mask;
}

void BaseBoard::set_piece_at(Square square, std::optional<Piece> piece, bool promoted_piece) {
    if (!piece.has_value()) {
        _remove_piece_at(square);
    } else {
        _set_piece_at(square, piece->type, piece->color, promoted_piece);
    }
}

void BaseBoard::set_board_fen(const std::string& fen) {
    _set_board_fen(fen);
}

std::map<Square, Piece> BaseBoard::piece_map(Bitboard mask) const {
    std::map<Square, Piece> result;
    for (Square square : scan_reversed(occupied & mask)) {
        result[square] = piece_at(square).value();
    }
    return result;
}

void BaseBoard::set_piece_map(const std::map<Square, Piece>& pieces) {
    _clear_board();
    for (auto& [square, piece] : pieces) {
        _set_piece_at(square, piece.type, piece.color);
    }
}

bool BaseBoard::operator==(const BaseBoard& other) const {
    return occupied == other.occupied &&
           occupied_co[intOf(Color::white)] == other.occupied_co[intOf(Color::white)] &&
           pawns == other.pawns &&
           knights == other.knights &&
           bishops == other.bishops &&
           rooks == other.rooks &&
           queens == other.queens &&
           kings == other.kings;
}

void BaseBoard::apply_transform(Bitboard (*f)(Bitboard)) {
    pawns   = f(pawns);
    knights = f(knights);
    bishops = f(bishops);
    rooks   = f(rooks);
    queens  = f(queens);
    kings   = f(kings);
    occupied_co[intOf(Color::white)] = f(occupied_co[intOf(Color::white)]);
    occupied_co[intOf(Color::black)] = f(occupied_co[intOf(Color::black)]);
    occupied = f(occupied);
    promoted = f(promoted);
}

BaseBoard BaseBoard::transform(Bitboard (*f)(Bitboard)) const {
    BaseBoard board = copy();
    board.apply_transform(f);
    return board;
}

void BaseBoard::apply_mirror() {
    apply_transform(flip_vertical);
    std::swap(occupied_co[intOf(Color::white)], occupied_co[intOf(Color::black)]);
}

BaseBoard BaseBoard::mirror() const {
    BaseBoard board = copy();
    board.apply_mirror();
    return board;
}

BaseBoard BaseBoard::copy() const {
    BaseBoard board(std::nullopt);
    board.pawns   = pawns;
    board.knights = knights;
    board.bishops = bishops;
    board.rooks   = rooks;
    board.queens  = queens;
    board.kings   = kings;
    board.occupied_co[intOf(Color::white)] = occupied_co[intOf(Color::white)];
    board.occupied_co[intOf(Color::black)] = occupied_co[intOf(Color::black)];
    board.occupied = occupied;
    board.promoted = promoted;
    return board;
}

BaseBoard BaseBoard::empty() {
    return BaseBoard(std::nullopt);
}

void BaseBoard::_reset_board() {
    pawns   = BB_RANK_2 | BB_RANK_7;
    knights = BB_B1 | BB_G1 | BB_B8 | BB_G8;
    bishops = BB_C1 | BB_F1 | BB_C8 | BB_F8;
    rooks   = BB_CORNERS;
    queens  = BB_D1 | BB_D8;
    kings   = BB_E1 | BB_E8;

    promoted = BB_EMPTY;

    occupied_co[intOf(Color::white)] = BB_RANK_1 | BB_RANK_2;
    occupied_co[intOf(Color::black)] = BB_RANK_7 | BB_RANK_8;
    occupied = BB_RANK_1 | BB_RANK_2 | BB_RANK_7 | BB_RANK_8;
}

void BaseBoard::_clear_board() {
    pawns = knights = bishops = rooks = queens = kings = BB_EMPTY;
    promoted = BB_EMPTY;
    occupied_co[intOf(Color::white)] = occupied_co[intOf(Color::black)] = BB_EMPTY;
    occupied = BB_EMPTY;
}

void BaseBoard::_set_board_fen(const std::string& fen_str) {
    std::string fen = fen_str;
    while (!fen.empty() && (fen.front() == ' ' || fen.front() == '\t')) fen.erase(fen.begin());
    while (!fen.empty() && (fen.back() == ' ' || fen.back() == '\t')) fen.pop_back();

    if (fen.find(' ') != std::string::npos)
        throw std::invalid_argument("expected position part of fen, got multiple parts: " + fen);

    std::vector<std::string> rows;
    std::string row;
    for (char c : fen) {
        if (c == '/') { rows.push_back(row); row.clear(); }
        else row.push_back(c);
    }
    rows.push_back(row);

    if (rows.size() != 8)
        throw std::invalid_argument("expected 8 rows in position part of fen: " + fen);

    static const std::string piece_symbols = "pnbrqk";

    for (auto& r : rows) {
        int field_sum = 0;
        bool prev_digit = false, prev_piece = false;
        for (char c : r) {
            if (c >= '1' && c <= '8') {
                if (prev_digit) throw std::invalid_argument("two subsequent digits in position part of fen: " + fen);
                field_sum += c - '0';
                prev_digit = true; prev_piece = false;
            } else if (c == '~') {
                if (!prev_piece) throw std::invalid_argument("'~' not after piece in position part of fen: " + fen);
                prev_digit = false; prev_piece = false;
            } else if (piece_symbols.find(std::tolower(c)) != std::string::npos) {
                field_sum += 1;
                prev_digit = false; prev_piece = true;
            } else {
                throw std::invalid_argument("invalid character in position part of fen: " + fen);
            }
        }
        if (field_sum != 8)
            throw std::invalid_argument("expected 8 columns per row in position part of fen: " + fen);
    }

    _clear_board();

    int rank_idx = 0;
    int file_idx = 0;
    for (char c : fen) {
        if (c == '/') {
            rank_idx++;
            file_idx = 0;
        } else if (c >= '1' && c <= '8') {
            file_idx += c - '0';
        } else if (c == '~') {
            promoted |= BB_SQUARES(SQUARES_180[rank_idx * 8 + file_idx - 1]);
        } else if (piece_symbols.find((char)(std::tolower(c))) != std::string::npos) {
            Square sq = SQUARES_180[rank_idx * 8 + file_idx];
            bool is_white = std::isupper(c);
            char lower = (char)(std::tolower(c));
            PieceType pt;
            if      (lower == 'p') pt = PieceType::pawn;
            else if (lower == 'n') pt = PieceType::knight;
            else if (lower == 'b') pt = PieceType::bishop;
            else if (lower == 'r') pt = PieceType::rook;
            else if (lower == 'q') pt = PieceType::queen;
            else                   pt = PieceType::king;
            _set_piece_at(sq, pt, is_white ? Color::white : Color::black);
            file_idx++;
        }
    }
}

BaseBoard::BaseBoard(const std::optional<std::string> &board_fen) {
    occupied_co = {BB_EMPTY, BB_EMPTY};
    if (!board_fen.has_value()) {
        _clear_board();
    } else if (board_fen.value() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR") {
        _reset_board();
    } else {
        _set_board_fen(board_fen.value());
    }
}


NAMESPACE_END
