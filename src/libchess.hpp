/*
Derived from python-chess
Copyright (C) 2012-2021 Niklas Fiekas

Translated to C++ by typeulli (2026)
Licensed under GPL v3
*/
#pragma once
#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <iterator>
#include <optional>

#define NAMESPACE_BEGIN namespace chess {
#define NAMESPACE_END }

#if defined(_WIN32)
  #ifdef BUILD_DLL
    #define API __declspec(dllexport)
  #else
    #define API __declspec(dllimport)
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define API __attribute__((visibility("default")))
#else
  #define API
#endif

#define EXTERN_API extern API

#if defined(__clang__) || defined(__GNUC__)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

NAMESPACE_BEGIN
using std::vector;
using std::map;
using std::pair;
using std::array;
enum class Color : uint8_t {
    white = true,
    black = false,
};
EXTERN_API inline std::string to_string(Color color);
Color operator~(Color color);

EXTERN_API inline int intOf(Color color);

enum class PieceType : uint8_t {
    none = 0,
    pawn = 1,
    knight = 2,
    bishop = 3,
    rook = 4,
    queen = 5,
    king = 6
};
EXTERN_API inline std::string to_string(PieceType piece_type);
EXTERN_API inline int intOf(PieceType piece_type);

enum class status : uint32_t {
    VALID = 0,
    NO_WHITE_KING = 1 << 0,
    NO_BLACK_KING = 1 << 1,
    TOO_MANY_KINGS = 1 << 2,
    TOO_MANY_WHITE_PAWNS = 1 << 3,
    TOO_MANY_BLACK_PAWNS = 1 << 4,
    PAWNS_ON_BACKRANK = 1 << 5,
    TOO_MANY_WHITE_PIECES = 1 << 6,
    TOO_MANY_BLACK_PIECES = 1 << 7,
    BAD_CASTLING_RIGHTS = 1 << 8,
    INVALID_EP_SQUARE = 1 << 9,
    OPPOSITE_CHECK = 1 << 10,
    EMPTY = 1 << 11,
    RACE_CHECK = 1 << 12,
    RACE_OVER = 1 << 13,
    RACE_MATERIAL = 1 << 14,
    TOO_MANY_CHECKERS = 1 << 15,
    IMPOSSIBLE_CHECK = 1 << 16
};

enum class Termination: uint8_t {
    CHECKMATE = 0,
    STALEMATE = 1,
    INSUFFICIENT_MATERIAL = 2,
    SEVENTYFIVE_MOVES = 3,
    FIVEFOLD_REPETITION = 4,
    FIFTY_MOVES = 5,
    THREEFOLD_REPETITION = 6,
    VARIANT_WIN = 7,
    VARIANT_LOSS = 8,
    VARIANT_DRAW = 9
};
EXTERN_API inline std::string to_string(Termination termination);

struct Outcome {
    Termination termination;
    Color winner;
};
EXTERN_API inline std::string to_string(Outcome outcome);

enum class Square : uint8_t {
    A1 =  0, B1 =  1, C1 =  2, D1 =  3, E1 =  4, F1 =  5, G1 =  6, H1 =  7,
    A2 =  8, B2 =  9, C2 = 10, D2 = 11, E2 = 12, F2 = 13, G2 = 14, H2 = 15,
    A3 = 16, B3 = 17, C3 = 18, D3 = 19, E3 = 20, F3 = 21, G3 = 22, H3 = 23,
    A4 = 24, B4 = 25, C4 = 26, D4 = 27, E4 = 28, F4 = 29, G4 = 30, H4 = 31,
    A5 = 32, B5 = 33, C5 = 34, D5 = 35, E5 = 36, F5 = 37, G5 = 38, H5 = 39,
    A6 = 40, B6 = 41, C6 = 42, D6 = 43, E6 = 44, F6 = 45, G6 = 46, H6 = 47,
    A7 = 48, B7 = 49, C7 = 50, D7 = 51, E7 = 52, F7 = 53, G7 = 54, H7 = 55,
    A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63
};
EXTERN_API inline std::string to_string(Square square);
EXTERN_API inline int intOf(Square square);
EXTERN_API Square& operator++(Square& square);

EXTERN_API Square parse_square(const char* str);
EXTERN_API Square at(int file, int rank);
EXTERN_API uint8_t file(Square square);
EXTERN_API uint8_t rank(Square square);
EXTERN_API uint8_t square_distance(Square a, Square b);
EXTERN_API uint8_t square_manhattan_distance(Square a, Square b);
EXTERN_API uint8_t square_knight_distance(Square a, Square b);
EXTERN_API Square mirror(Square square);
constexpr Square SQUARES_180[64] = {
    Square::A8, Square::B8, Square::C8, Square::D8, Square::E8, Square::F8, Square::G8, Square::H8,
    Square::A7, Square::B7, Square::C7, Square::D7, Square::E7, Square::F7, Square::G7, Square::H7,
    Square::A6, Square::B6, Square::C6, Square::D6, Square::E6, Square::F6, Square::G6, Square::H6,
    Square::A5, Square::B5, Square::C5, Square::D5, Square::E5, Square::F5, Square::G5, Square::H5,
    Square::A4, Square::B4, Square::C4, Square::D4, Square::E4, Square::F4, Square::G4, Square::H4,
    Square::A3, Square::B3, Square::C3, Square::D3, Square::E3, Square::F3, Square::G3, Square::H3,
    Square::A2, Square::B2, Square::C2, Square::D2, Square::E2, Square::F2, Square::G2, Square::H2,
    Square::A1, Square::B1, Square::C1, Square::D1, Square::E1, Square::F1, Square::G1, Square::H1,
};

typedef uint64_t Bitboard;
constexpr Bitboard BB_EMPTY = 0ULL;
constexpr Bitboard BB_ALL = 0xFFFFFFFFFFFFFFFFULL;
constexpr Bitboard BB_SQUARES(Square square) {
    return 1ULL << static_cast<uint8_t>(square);
}
constexpr Bitboard BB_LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;
constexpr Bitboard BB_DARK_SQUARES = 0xAA55AA55AA55AA55ULL;
constexpr Bitboard _gen_BB_FILES(uint8_t file) { return 0x0101010101010101ULL << file; }
constexpr Bitboard _gen_BB_RANKS(uint8_t rank) { return 0xFFULL << (rank << 3); }

constexpr Bitboard BB_A1 = 1ULL << 0;
constexpr Bitboard BB_B1 = 1ULL << 1;
constexpr Bitboard BB_C1 = 1ULL << 2;
constexpr Bitboard BB_D1 = 1ULL << 3;
constexpr Bitboard BB_E1 = 1ULL << 4;
constexpr Bitboard BB_F1 = 1ULL << 5;
constexpr Bitboard BB_G1 = 1ULL << 6;
constexpr Bitboard BB_H1 = 1ULL << 7;
constexpr Bitboard BB_A2 = 1ULL << 8;
constexpr Bitboard BB_B2 = 1ULL << 9;
constexpr Bitboard BB_C2 = 1ULL << 10;
constexpr Bitboard BB_D2 = 1ULL << 11;
constexpr Bitboard BB_E2 = 1ULL << 12;
constexpr Bitboard BB_F2 = 1ULL << 13;
constexpr Bitboard BB_G2 = 1ULL << 14;
constexpr Bitboard BB_H2 = 1ULL << 15;
constexpr Bitboard BB_A3 = 1ULL << 16;
constexpr Bitboard BB_B3 = 1ULL << 17;
constexpr Bitboard BB_C3 = 1ULL << 18;
constexpr Bitboard BB_D3 = 1ULL << 19;
constexpr Bitboard BB_E3 = 1ULL << 20;
constexpr Bitboard BB_F3 = 1ULL << 21;
constexpr Bitboard BB_G3 = 1ULL << 22;
constexpr Bitboard BB_H3 = 1ULL << 23;
constexpr Bitboard BB_A4 = 1ULL << 24;
constexpr Bitboard BB_B4 = 1ULL << 25;
constexpr Bitboard BB_C4 = 1ULL << 26;
constexpr Bitboard BB_D4 = 1ULL << 27;
constexpr Bitboard BB_E4 = 1ULL << 28;
constexpr Bitboard BB_F4 = 1ULL << 29;
constexpr Bitboard BB_G4 = 1ULL << 30;
constexpr Bitboard BB_H4 = 1ULL << 31;
constexpr Bitboard BB_A5 = 1ULL << 32;
constexpr Bitboard BB_B5 = 1ULL << 33;
constexpr Bitboard BB_C5 = 1ULL << 34;
constexpr Bitboard BB_D5 = 1ULL << 35;
constexpr Bitboard BB_E5 = 1ULL << 36;
constexpr Bitboard BB_F5 = 1ULL << 37;
constexpr Bitboard BB_G5 = 1ULL << 38;
constexpr Bitboard BB_H5 = 1ULL << 39;
constexpr Bitboard BB_A6 = 1ULL << 40;
constexpr Bitboard BB_B6 = 1ULL << 41;
constexpr Bitboard BB_C6 = 1ULL << 42;
constexpr Bitboard BB_D6 = 1ULL << 43;
constexpr Bitboard BB_E6 = 1ULL << 44;
constexpr Bitboard BB_F6 = 1ULL << 45;
constexpr Bitboard BB_G6 = 1ULL << 46;
constexpr Bitboard BB_H6 = 1ULL << 47;
constexpr Bitboard BB_A7 = 1ULL << 48;
constexpr Bitboard BB_B7 = 1ULL << 49;
constexpr Bitboard BB_C7 = 1ULL << 50;
constexpr Bitboard BB_D7 = 1ULL << 51;
constexpr Bitboard BB_E7 = 1ULL << 52;
constexpr Bitboard BB_F7 = 1ULL << 53;
constexpr Bitboard BB_G7 = 1ULL << 54;
constexpr Bitboard BB_H7 = 1ULL << 55;
constexpr Bitboard BB_A8 = 1ULL << 56;
constexpr Bitboard BB_B8 = 1ULL << 57;
constexpr Bitboard BB_C8 = 1ULL << 58;
constexpr Bitboard BB_D8 = 1ULL << 59;
constexpr Bitboard BB_E8 = 1ULL << 60;
constexpr Bitboard BB_F8 = 1ULL << 61;
constexpr Bitboard BB_G8 = 1ULL << 62;
constexpr Bitboard BB_H8 = 1ULL << 63;
constexpr Bitboard BB_CORNERS = BB_A1 | BB_H1 | BB_A8 | BB_H8;
constexpr Bitboard BB_CENTER = BB_D4 | BB_E4 | BB_D5 | BB_E5;

constexpr Bitboard BB_FILE_A = _gen_BB_FILES(0);
constexpr Bitboard BB_FILE_B = _gen_BB_FILES(1);
constexpr Bitboard BB_FILE_C = _gen_BB_FILES(2);
constexpr Bitboard BB_FILE_D = _gen_BB_FILES(3);
constexpr Bitboard BB_FILE_E = _gen_BB_FILES(4);
constexpr Bitboard BB_FILE_F = _gen_BB_FILES(5);
constexpr Bitboard BB_FILE_G = _gen_BB_FILES(6);
constexpr Bitboard BB_FILE_H = _gen_BB_FILES(7);
EXTERN_API const Bitboard BB_FILES[8];

constexpr Bitboard BB_RANK_1 = _gen_BB_RANKS(0);
constexpr Bitboard BB_RANK_2 = _gen_BB_RANKS(1);
constexpr Bitboard BB_RANK_3 = _gen_BB_RANKS(2);
constexpr Bitboard BB_RANK_4 = _gen_BB_RANKS(3);
constexpr Bitboard BB_RANK_5 = _gen_BB_RANKS(4);
constexpr Bitboard BB_RANK_6 = _gen_BB_RANKS(5);
constexpr Bitboard BB_RANK_7 = _gen_BB_RANKS(6);
constexpr Bitboard BB_RANK_8 = _gen_BB_RANKS(7);
EXTERN_API const Bitboard BB_RANKS[8];

constexpr Bitboard BB_BACKRANKS = BB_RANK_1 | BB_RANK_8;
EXTERN_API int lsb(Bitboard x);
class API scan_forward {
    Bitboard bb;
public:
    explicit scan_forward(Bitboard b);
    struct API iterator {
        Bitboard bb;
        Square operator*() const;
        iterator& operator++();
        bool operator!=(const iterator& other) const;
    };

    NODISCARD iterator begin() const;
    static iterator end();
};
EXTERN_API int msb(Bitboard bb);
class API scan_reversed {
    Bitboard bb;

public:
    explicit scan_reversed(Bitboard b);
    struct API iterator {
        Bitboard bb;
        Square operator*() const;
        iterator& operator++();
        bool operator!=(const iterator& other) const;
    };

    NODISCARD iterator begin() const;
    static iterator end();
};


EXTERN_API Bitboard flip_vertical(Bitboard bb);
EXTERN_API Bitboard flip_horizontal(Bitboard bb);
EXTERN_API Bitboard flip_diagonal(Bitboard bb);
EXTERN_API Bitboard flip_anti_diagonal(Bitboard bb);

EXTERN_API Bitboard shift_down(Bitboard b);
EXTERN_API Bitboard shift_2_down(Bitboard b);
EXTERN_API Bitboard shift_up(Bitboard b);
EXTERN_API Bitboard shift_2_up(Bitboard b);
EXTERN_API Bitboard shift_right(Bitboard b);
EXTERN_API Bitboard shift_2_right(Bitboard b);
EXTERN_API Bitboard shift_left(Bitboard b);
EXTERN_API Bitboard shift_2_left(Bitboard b);
EXTERN_API Bitboard shift_up_left(Bitboard b);
EXTERN_API Bitboard shift_up_right(Bitboard b);
EXTERN_API Bitboard shift_down_left(Bitboard b);
EXTERN_API Bitboard shift_down_right(Bitboard b);

EXTERN_API Bitboard _sliding_attacks(Square square, Bitboard occupied, const std::vector<int>& deltas);
EXTERN_API Bitboard _step_attacks(Square square, const std::vector<int>& deltas);

EXTERN_API const std::array<Bitboard, 64> BB_KNIGHT_ATTACKS;
EXTERN_API const std::array<Bitboard, 64> BB_KING_ATTACKS;
EXTERN_API const std::array<std::array<Bitboard, 64>, 2>  BB_PAWN_ATTACKS;

EXTERN_API Bitboard _edges(Square square);

class _carry_rippler {
    Bitboard mask;
public:
    explicit _carry_rippler(Bitboard m);
    struct iterator {
        Bitboard mask;
        Bitboard subset;
        bool done;
        Bitboard operator*() const;
        iterator& operator++();
        bool operator!=(const iterator& other) const;
    };

    NODISCARD iterator begin() const;
    static iterator end();
};
EXTERN_API std::pair<
std::vector<Bitboard>,
std::vector<std::map<Bitboard, Bitboard>>
> _attack_table(const std::vector<int>& deltas);

EXTERN_API const std::vector<Bitboard> BB_DIAG_MASKS;
EXTERN_API const std::vector<Bitboard> BB_FILE_MASKS;
EXTERN_API const std::vector<Bitboard> BB_RANK_MASKS;

EXTERN_API const std::vector<std::map<Bitboard, Bitboard>> BB_FILE_ATTACKS;
EXTERN_API const std::vector<std::map<Bitboard, Bitboard>> BB_DIAG_ATTACKS;
EXTERN_API const std::vector<std::map<Bitboard, Bitboard>> BB_RANK_ATTACKS;

EXTERN_API vector<vector<Bitboard>> _rays();
EXTERN_API const vector<vector<Bitboard>> BB_RAYS;

EXTERN_API Bitboard ray(Square from, Square to);
EXTERN_API Bitboard between(Square from, Square to);

struct Piece {
    PieceType type;
    Color color;
};

struct Move {
    Square from;
    Square to;
    std::optional<PieceType> promotion = std::nullopt;
    std::optional<PieceType> drop = std::nullopt;
    Move() = delete;
    Move(Square from, Square to) : from(from), to(to) {}
    Move(Square from, Square to, PieceType promotion) : from(from), to(to), promotion(promotion) {}
    API NODISCARD inline static Move from_uci(const std::string& uci_str);
    API NODISCARD inline std::string uci() const;
};
EXTERN_API inline std::string to_string(const Move& move);

class API BaseBoard {
public:
    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;
    Bitboard promoted;
    std::array<Bitboard, 2> occupied_co;
    Bitboard occupied;

    BaseBoard(const std::optional<std::string> &board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    void reset_board();
    void clear_board();

    NODISCARD Bitboard pieces_mask(PieceType piece_type, Color color) const;
    NODISCARD Bitboard pieces(PieceType piece_type, Color color) const;

    NODISCARD std::optional<Piece> piece_at(Square square) const;
    NODISCARD std::optional<PieceType> piece_type_at(Square square) const;
    NODISCARD std::optional<Color> color_at(Square square) const;
    NODISCARD std::optional<Square> king(Color color) const;

    NODISCARD Bitboard attacks_mask(Square square) const;

    NODISCARD Bitboard _attackers_mask(Color color, Square square, Bitboard occ) const;

    NODISCARD Bitboard attackers_mask(Color color, Square square) const;
    NODISCARD Bitboard attackers_mask(Color color, Square square, Bitboard occupied) const;
    NODISCARD bool is_attacked_by(Color color, Square square) const;

    NODISCARD Bitboard pin_mask(Color color, Square square) const;
    NODISCARD bool is_pinned(Color color, Square square) const;

    std::optional<PieceType> _remove_piece_at(Square square);
    std::optional<Piece> remove_piece_at(Square square);
    void _set_piece_at(Square square, PieceType piece_type, Color color, bool promoted = false);
    void set_piece_at(Square square, std::optional<Piece> piece, bool promoted = false);

    void set_board_fen(const std::string& fen);

    NODISCARD std::map<Square, Piece> piece_map(Bitboard mask = BB_ALL) const;
    void set_piece_map(const std::map<Square, Piece>& pieces);

    bool operator==(const BaseBoard& other) const;
    void apply_transform(Bitboard (*f)(Bitboard));
    BaseBoard transform(Bitboard (*f)(Bitboard)) const;
    void apply_mirror();
    NODISCARD BaseBoard mirror() const;
    NODISCARD BaseBoard copy() const;

    static BaseBoard empty();
protected:
    void _reset_board();
    void _clear_board();
    void _set_board_fen(const std::string& fen);
};


struct BoardState {
    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;

    Bitboard occupied_w;
    Bitboard occupied_b;
    Bitboard occupied;

    Bitboard promoted;

    Bitboard castling_rights;
    int halfmove_clock;
    int fullmove_number;
    std::optional<Square> ep_square;
    Color turn;

    template<typename BoardT>
    BoardState(const BoardT& board)
        : pawns(board.pawns)
        , knights(board.knights)
        , bishops(board.bishops)
        , rooks(board.rooks)
        , queens(board.queens)
        , kings(board.kings)
        , occupied_w(board.occupied_co[intOf(Color::white)])
        , occupied_b(board.occupied_co[intOf(Color::black)])
        , occupied(board.occupied)
        , promoted(board.promoted)
        , castling_rights(board.castling_rights)
        , halfmove_clock(board.halfmove_clock)
        , fullmove_number(board.fullmove_number)
        , ep_square(board.ep_square)
        , turn(board.turn)
    {}

    template<typename BoardT>
    void restore(BoardT& board) const {
        board.pawns   = pawns;
        board.knights = knights;
        board.bishops = bishops;
        board.rooks   = rooks;
        board.queens  = queens;
        board.kings   = kings;
        board.occupied_co[intOf(Color::white)] = occupied_w;
        board.occupied_co[intOf(Color::black)] = occupied_b;
        board.occupied = occupied;
        board.promoted = promoted;
        board.turn = turn;
        board.castling_rights = castling_rights;
        board.ep_square = ep_square;
        board.halfmove_clock = halfmove_clock;
        board.fullmove_number = fullmove_number;
    }
};


class API Board : public BaseBoard {
public:
    static const std::string starting_fen;
    static constexpr bool one_king = true;
    static constexpr bool captures_compulsory = false;

    Bitboard castling_rights{};
    int fullmove_number{};
    int halfmove_clock{};
    vector<Move> move_stack;
    std::optional<Square> ep_square;
    Color turn;

    Board(std::optional<std::string> fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    void reset();
    NODISCARD void reset_board();
    void clear();
    NODISCARD void clear_board();
    void clear_stack();

    NODISCARD Board root() const;
    NODISCARD int ply() const;

    std::optional<Piece> remove_piece_at(Square square);
    void set_piece_at(Square square, std::optional<Piece> piece, bool promoted = false);

    NODISCARD vector<Move> generate_pseudo_legal_moves(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD vector<Move> generate_pseudo_legal_ep(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD vector<Move> generate_pseudo_legal_captures(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;

    NODISCARD Bitboard checkers_mask() const;
    NODISCARD bool is_check() const;
    NODISCARD bool gives_check(const Move& move);
    NODISCARD bool is_into_check(const Move& move) const;
    NODISCARD bool was_into_check() const;

    NODISCARD bool is_pseudo_legal(const Move& move) const;
    NODISCARD bool is_legal(const Move& move) const;

    NODISCARD virtual bool is_variant_end() const;
    NODISCARD virtual bool is_variant_loss() const;
    NODISCARD virtual bool is_variant_win() const;
    NODISCARD virtual bool is_variant_draw() const;

    bool is_game_over(bool claim_draw = false);
    std::string result(bool claim_draw = false);
    std::optional<Outcome> outcome(bool claim_draw = false);

    NODISCARD bool is_checkmate() const;
    NODISCARD bool is_stalemate() const;
    NODISCARD bool is_insufficient_material() const;
    NODISCARD bool has_insufficient_material(Color color) const;

    NODISCARD bool is_seventyfive_moves() const;
    NODISCARD bool is_fivefold_repetition() const;
    bool can_claim_draw();
    NODISCARD bool is_fifty_moves() const;
    bool can_claim_fifty_moves();
    bool can_claim_threefold_repetition();
    NODISCARD bool is_repetition(int count = 3) const;

    void push(Move move);
    Move pop();
    NODISCARD Move peek() const;

    NODISCARD Move find_move(Square from_square, Square to_square, std::optional<PieceType> promotion = std::nullopt) const;

    NODISCARD std::string castling_shredder_fen() const;
    NODISCARD std::string castling_xfen() const;

    NODISCARD bool has_pseudo_legal_en_passant() const;
    NODISCARD bool has_legal_en_passant() const;

    NODISCARD std::string fen(bool shredder = false, const std::string& en_passant = "legal", std::optional<bool> promoted = std::nullopt) const;
    NODISCARD std::string shredder_fen(const std::string& en_passant = "legal", std::optional<bool> promoted = std::nullopt) const;

    void set_fen(const std::string& fen);
    void set_castling_fen(const std::string& castling_fen);
    void set_board_fen(const std::string& fen);
    void set_piece_map(const std::map<Square, Piece>& pieces);

    NODISCARD std::string epd(bool shredder = false, const std::string& en_passant = "legal", std::optional<bool> promoted = std::nullopt) const;
    std::map<std::string, std::string> set_epd(const std::string& epd);

    std::string san(const Move& move);
    std::string lan(const Move& move);
    std::string san_and_push(const Move& move);
    NODISCARD std::string variation_san(const vector<Move>& variation) const;

    NODISCARD Move parse_san(const std::string& san) const;
    Move push_san(const std::string& san);

    NODISCARD std::string uci(const Move& move, std::optional<bool> chess960 = std::nullopt) const;
    Move parse_uci(const std::string& uci);
    Move push_uci(const std::string& uci);

    NODISCARD bool is_en_passant(const Move& move) const;
    NODISCARD bool is_capture(const Move& move) const;
    NODISCARD bool is_zeroing(const Move& move) const;
    NODISCARD bool is_irreversible(const Move& move) const;
    NODISCARD bool is_castling(const Move& move) const;
    NODISCARD bool is_kingside_castling(const Move& move) const;
    NODISCARD bool is_queenside_castling(const Move& move) const;

    NODISCARD Bitboard clean_castling_rights() const;
    NODISCARD bool has_castling_rights(Color color) const;
    NODISCARD bool has_kingside_castling_rights(Color color) const;
    NODISCARD bool has_queenside_castling_rights(Color color) const;

    NODISCARD status get_status() const;
    NODISCARD bool is_valid() const;

    NODISCARD vector<Move> generate_legal_moves(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD vector<Move> generate_legal_ep(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD vector<Move> generate_legal_captures(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD vector<Move> generate_castling_moves(Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;

    bool operator==(const Board& other) const;

    NODISCARD void apply_transform(Bitboard (*f)(Bitboard));
    NODISCARD Board transform(Bitboard (*f)(Bitboard)) const;
    NODISCARD void apply_mirror();
    NODISCARD Board mirror() const;
    NODISCARD Board copy(bool stack = true) const;

    static Board empty();

protected:
    vector<BoardState> _stack;

    NODISCARD bool _is_halfmoves(int n) const;
    NODISCARD BoardState _board_state() const;
    virtual void _push_capture(const Move& move, Square capture_square, PieceType piece_type, bool was_promoted);

    NODISCARD std::optional<Square> _valid_ep_square() const;
    NODISCARD bool _ep_skewered(Square king, Square capturer) const;
    NODISCARD Bitboard _slider_blockers(Square king) const;
    NODISCARD bool _is_safe(Square king, Bitboard blockers, const Move& move) const;
    NODISCARD vector<Move> _generate_evasions(Square king, Bitboard checkers, Bitboard from_mask = BB_ALL, Bitboard to_mask = BB_ALL) const;
    NODISCARD bool _attacked_for_king(Bitboard path, Bitboard occupied) const;
    NODISCARD bool _reduces_castling_rights(const Move& move) const;

    void _set_castling_fen(const std::string& castling_fen);

    NODISCARD Move _from_chess960(bool chess960, Square from_square, Square to_square,
                        std::optional<PieceType> promotion = std::nullopt,
                        std::optional<PieceType> drop = std::nullopt) const;
    NODISCARD Move _to_chess960(const Move& move) const;

    std::string _algebraic(const Move& move, bool long_notation = false);
    std::string _algebraic_and_push(const Move& move, bool long_notation = false);
    NODISCARD std::string _algebraic_without_suffix(const Move& move, bool long_notation = false) const;

    NODISCARD std::tuple<Bitboard, Bitboard, Bitboard, Bitboard, Bitboard, Bitboard,
               Bitboard, Bitboard, Color, Bitboard, std::optional<Square>>
    _transposition_key() const;
};

API std::ostream& operator<<(std::ostream& os, const Board& board);
NAMESPACE_END