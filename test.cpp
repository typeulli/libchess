#include <gtest/gtest.h>
#include "src/libchess.hpp"

using namespace chess;

// ============================================================
// Square utilities
// ============================================================

TEST(SquareTest, AtFileRank) {
    EXPECT_EQ(at(0, 0), Square::A1);
    EXPECT_EQ(at(7, 7), Square::H8);
    EXPECT_EQ(at(4, 0), Square::E1);
    EXPECT_EQ(at(0, 7), Square::A8);
}

TEST(SquareTest, FileAndRank) {
    EXPECT_EQ(file(Square::A1), 0);
    EXPECT_EQ(file(Square::H1), 7);
    EXPECT_EQ(rank(Square::A1), 0);
    EXPECT_EQ(rank(Square::A8), 7);
    EXPECT_EQ(file(Square::E4), 4);
    EXPECT_EQ(rank(Square::E4), 3);
}

TEST(SquareTest, ParseSquare) {
    EXPECT_EQ(parse_square("a1"), Square::A1);
    EXPECT_EQ(parse_square("h8"), Square::H8);
    EXPECT_EQ(parse_square("e4"), Square::E4);
    EXPECT_EQ(parse_square("d5"), Square::D5);
}

TEST(SquareTest, Mirror) {
    EXPECT_EQ(mirror(Square::A1), Square::A8);
    EXPECT_EQ(mirror(Square::H8), Square::H1);
    EXPECT_EQ(mirror(Square::E4), Square::E5);
}

TEST(SquareTest, Distance) {
    EXPECT_EQ(square_distance(Square::A1, Square::A1), 0);
    EXPECT_EQ(square_distance(Square::A1, Square::H8), 7);  // Chebyshev distance
    EXPECT_EQ(square_distance(Square::E4, Square::E5), 1);
}

TEST(SquareTest, ManhattanDistance) {
    EXPECT_EQ(square_manhattan_distance(Square::A1, Square::A1), 0);
    EXPECT_EQ(square_manhattan_distance(Square::A1, Square::B2), 2);
    EXPECT_EQ(square_manhattan_distance(Square::A1, Square::H8), 14);
}

TEST(SquareTest, IncrementOperator) {
    Square s = Square::A1;
    ++s;
    EXPECT_EQ(s, Square::B1);
    ++s;
    EXPECT_EQ(s, Square::C1);
}

// ============================================================
// Bitboard helpers
// ============================================================

TEST(BitboardTest, BB_SQUARES) {
    EXPECT_EQ(BB_SQUARES(Square::A1), 1ULL);
    EXPECT_EQ(BB_SQUARES(Square::H8), 1ULL << 63);
    EXPECT_EQ(BB_SQUARES(Square::E1), BB_E1);
}

TEST(BitboardTest, ShiftUp) {
    Bitboard rank1 = BB_RANK_1;
    EXPECT_EQ(shift_up(rank1), BB_RANK_2);
}

TEST(BitboardTest, ShiftDown) {
    Bitboard rank2 = BB_RANK_2;
    EXPECT_EQ(shift_down(rank2), BB_RANK_1);
    EXPECT_EQ(shift_down(BB_RANK_1), BB_EMPTY);
}

TEST(BitboardTest, ShiftLeft) {
    Bitboard fileB = BB_FILE_B;
    EXPECT_EQ(shift_left(fileB), BB_FILE_A);
}

TEST(BitboardTest, ShiftRight) {
    Bitboard fileA = BB_FILE_A;
    EXPECT_EQ(shift_right(fileA), BB_FILE_B);
}

TEST(BitboardTest, Shift2Up) {
    EXPECT_EQ(shift_2_up(BB_RANK_2), BB_RANK_4);
}

TEST(BitboardTest, Shift2Down) {
    EXPECT_EQ(shift_2_down(BB_RANK_4), BB_RANK_2);
}

TEST(BitboardTest, FlipVertical) {
    EXPECT_EQ(flip_vertical(BB_RANK_1), BB_RANK_8);
    EXPECT_EQ(flip_vertical(BB_RANK_8), BB_RANK_1);
}

TEST(BitboardTest, FlipHorizontal) {
    EXPECT_EQ(flip_horizontal(BB_FILE_A), BB_FILE_H);
    EXPECT_EQ(flip_horizontal(BB_FILE_H), BB_FILE_A);
}

TEST(BitboardTest, RayBetween) {
    // ray from A1 to H8 should include all diagonal squares
    Bitboard r = ray(Square::A1, Square::H8);
    EXPECT_TRUE(r & BB_SQUARES(Square::D4));

    // between A1 and D4 should include B2 and C3 but not A1 or D4
    Bitboard b = between(Square::A1, Square::D4);
    EXPECT_TRUE(b & BB_SQUARES(Square::B2));
    EXPECT_TRUE(b & BB_SQUARES(Square::C3));
    EXPECT_FALSE(b & BB_SQUARES(Square::A1));
    EXPECT_FALSE(b & BB_SQUARES(Square::D4));
}

TEST(BitboardTest, KnightAttacks) {
    // Knight on E4 (index 28) can attack 8 squares
    Bitboard attacks = BB_KNIGHT_ATTACKS[intOf(Square::E4)];
    EXPECT_EQ(std::popcount(attacks), 8);

    // Knight on A1 can only attack 2 squares
    Bitboard corner = BB_KNIGHT_ATTACKS[intOf(Square::A1)];
    EXPECT_EQ(std::popcount(corner), 2);
}

TEST(BitboardTest, KingAttacks) {
    // King on E4 attacks 8 squares
    EXPECT_EQ(std::popcount(BB_KING_ATTACKS[intOf(Square::E4)]), 8);
    // King on A1 attacks 3 squares
    EXPECT_EQ(std::popcount(BB_KING_ATTACKS[intOf(Square::A1)]), 3);
}

TEST(BitboardTest, PawnAttacks) {
    // White pawn on E2 attacks D3 and F3
    Bitboard wp = BB_PAWN_ATTACKS[intOf(Color::white)][intOf(Square::E2)];
    EXPECT_TRUE(wp & BB_SQUARES(Square::D3));
    EXPECT_TRUE(wp & BB_SQUARES(Square::F3));
    EXPECT_EQ(std::popcount(wp), 2);

    // Black pawn on E7 attacks D6 and F6
    Bitboard bp = BB_PAWN_ATTACKS[intOf(Color::black)][intOf(Square::E7)];
    EXPECT_TRUE(bp & BB_SQUARES(Square::D6));
    EXPECT_TRUE(bp & BB_SQUARES(Square::F6));
}

// ============================================================
// scan_forward / scan_reversed iterators
// ============================================================

TEST(ScanTest, ScanForwardEmpty) {
    int count = 0;
    for (Square s : scan_forward(BB_EMPTY)) {
        (void)s;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(ScanTest, ScanForwardAllSquares) {
    int count = 0;
    for (Square s : scan_forward(BB_ALL)) {
        (void)s;
        ++count;
    }
    EXPECT_EQ(count, 64);
}

TEST(ScanTest, ScanForwardOrder) {
    Bitboard bb = BB_SQUARES(Square::A1) | BB_SQUARES(Square::H8);
    auto it = scan_forward(bb).begin();
    EXPECT_EQ(*it, Square::A1);
}

TEST(ScanTest, ScanReversedOrder) {
    Bitboard bb = BB_SQUARES(Square::A1) | BB_SQUARES(Square::H8);
    auto it = scan_reversed(bb).begin();
    EXPECT_EQ(*it, Square::H8);
}

// ============================================================
// BaseBoard construction & piece queries
// ============================================================

class BaseBoardTest : public ::testing::Test {
protected:
    BaseBoard board;  // default: starting position
};

TEST_F(BaseBoardTest, DefaultPositionOccupied) {
    // Ranks 1-2 and 7-8 should be occupied; ranks 3-6 empty
    EXPECT_EQ(board.occupied & BB_RANK_3, BB_EMPTY);
    EXPECT_NE(board.occupied & BB_RANK_1, BB_EMPTY);
    EXPECT_NE(board.occupied & BB_RANK_8, BB_EMPTY);
}

TEST_F(BaseBoardTest, KingSquares) {
    auto wk = board.king(Color::white);
    auto bk = board.king(Color::black);
    ASSERT_TRUE(wk.has_value());
    ASSERT_TRUE(bk.has_value());
    EXPECT_EQ(*wk, Square::E1);
    EXPECT_EQ(*bk, Square::E8);
}

TEST_F(BaseBoardTest, PieceAt) {
    auto p = board.piece_at(Square::E1);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->type, PieceType::king);
    EXPECT_EQ(p->color, Color::white);
}

TEST_F(BaseBoardTest, NoPieceAtEmpty) {
    EXPECT_FALSE(board.piece_at(Square::E4).has_value());
}

TEST_F(BaseBoardTest, PieceTypeAt) {
    EXPECT_EQ(*board.piece_type_at(Square::A1), PieceType::rook);
    EXPECT_EQ(*board.piece_type_at(Square::B1), PieceType::knight);
    EXPECT_EQ(*board.piece_type_at(Square::C1), PieceType::bishop);
    EXPECT_EQ(*board.piece_type_at(Square::D1), PieceType::queen);
    EXPECT_EQ(*board.piece_type_at(Square::A2), PieceType::pawn);
}

TEST_F(BaseBoardTest, ColorAt) {
    EXPECT_EQ(*board.color_at(Square::E1), Color::white);
    EXPECT_EQ(*board.color_at(Square::E8), Color::black);
    EXPECT_FALSE(board.color_at(Square::E4).has_value());
}

TEST_F(BaseBoardTest, PiecesMask) {
    Bitboard wPawns = board.pieces_mask(PieceType::pawn, Color::white);
    EXPECT_EQ(wPawns, BB_RANK_2);

    Bitboard bPawns = board.pieces_mask(PieceType::pawn, Color::black);
    EXPECT_EQ(bPawns, BB_RANK_7);
}

TEST_F(BaseBoardTest, PieceMap) {
    auto pm = board.piece_map();
    EXPECT_EQ(pm.size(), 32u);  // 32 pieces in starting position
}

// ============================================================
// BaseBoard mutation
// ============================================================

TEST(BaseBoardMutationTest, SetAndRemovePiece) {
    BaseBoard board = BaseBoard::empty();
    board.set_piece_at(Square::E4, Piece{PieceType::knight, Color::white});

    ASSERT_TRUE(board.piece_at(Square::E4).has_value());
    EXPECT_EQ(board.piece_at(Square::E4)->type, PieceType::knight);

    board.remove_piece_at(Square::E4);
    EXPECT_FALSE(board.piece_at(Square::E4).has_value());
}

TEST(BaseBoardMutationTest, ClearBoard) {
    BaseBoard board;
    board.clear_board();
    EXPECT_EQ(board.occupied, BB_EMPTY);
    EXPECT_FALSE(board.king(Color::white).has_value());
}

TEST(BaseBoardMutationTest, ResetBoard) {
    BaseBoard board;
    board.clear_board();
    board.reset_board();
    // After reset, kings should be back
    EXPECT_EQ(*board.king(Color::white), Square::E1);
    EXPECT_EQ(*board.king(Color::black), Square::E8);
}

TEST(BaseBoardMutationTest, SetBoardFen) {
    BaseBoard board;
    // Kiwipete position
    board.set_board_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R");
    EXPECT_EQ(*board.king(Color::white), Square::E1);
    EXPECT_EQ(*board.king(Color::black), Square::E8);
}

// ============================================================
// BaseBoard attacks & pins
// ============================================================

TEST(BaseBoardAttackTest, AttacksMaskRook) {
    BaseBoard board = BaseBoard::empty();
    board.set_piece_at(Square::E4, Piece{PieceType::rook, Color::white});
    Bitboard attacks = board.attacks_mask(Square::E4);
    // Rook on empty board hits all of file E and rank 4 minus E4 itself
    EXPECT_TRUE(attacks & BB_SQUARES(Square::E1));
    EXPECT_TRUE(attacks & BB_SQUARES(Square::E8));
    EXPECT_TRUE(attacks & BB_SQUARES(Square::A4));
    EXPECT_TRUE(attacks & BB_SQUARES(Square::H4));
    EXPECT_FALSE(attacks & BB_SQUARES(Square::E4));
}

TEST(BaseBoardAttackTest, IsAttackedBy) {
    BaseBoard board;
    board.set_board_fen("rnbqkbnr/pppppppp/8/8/8/5P1P/PPPPP1P1/RNBQKBNR");

    EXPECT_TRUE(board.is_attacked_by(Color::white, Square::D3));
    EXPECT_TRUE(board.is_attacked_by(Color::white, Square::F3));
    EXPECT_FALSE(board.is_attacked_by(Color::white, Square::G3));
}

TEST(BaseBoardAttackTest, AttackersMask) {
    BaseBoard board = BaseBoard::empty();
    board.set_piece_at(Square::E1, Piece{PieceType::king, Color::white});
    board.set_piece_at(Square::E8, Piece{PieceType::king, Color::black});
    board.set_piece_at(Square::A4, Piece{PieceType::rook, Color::white});
    board.set_piece_at(Square::H4, Piece{PieceType::rook, Color::black});

    // White rook on A4 attacks H4
    Bitboard attackers = board.attackers_mask(Color::white, Square::H4);
    EXPECT_TRUE(attackers & BB_SQUARES(Square::A4));
}

TEST(BaseBoardAttackTest, PinDetection) {
    BaseBoard b = BaseBoard::empty();

    b.set_piece_at(Square::E1, {{PieceType::king, Color::white}});
    b.set_piece_at(Square::E2, {{PieceType::rook, Color::white}});
    b.set_piece_at(Square::E8, {{PieceType::rook, Color::black}});

    Bitboard pin = b.pin_mask(Color::white, Square::E2);

    EXPECT_TRUE(pin & BB_FILE_E);
}

// ============================================================
// BaseBoard copy, equality, mirror, transform
// ============================================================

TEST(BaseBoardTransformTest, Equality) {
    BaseBoard a, b;
    EXPECT_TRUE(a == b);
    b.remove_piece_at(Square::E1);
    EXPECT_FALSE(a == b);
}

TEST(BaseBoardTransformTest, Copy) {
    BaseBoard original;
    BaseBoard copy = original.copy();
    EXPECT_TRUE(original == copy);
    copy.clear_board();
    EXPECT_FALSE(original == copy);  // original unaffected
}

TEST(BaseBoardTransformTest, Mirror) {
    BaseBoard board;
    board.set_board_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR");
    BaseBoard mirrored = board.mirror();
    EXPECT_EQ(mirrored.piece_at(Square::E5)->type, PieceType::pawn);
}

TEST(BaseBoardTransformTest, EmptyFactory) {
    BaseBoard empty = BaseBoard::empty();
    EXPECT_EQ(empty.occupied, BB_EMPTY);
}

// ============================================================
// intOf helpers
// ============================================================

TEST(IntOfTest, Color) {
    EXPECT_EQ(intOf(Color::white), 1);
    EXPECT_EQ(intOf(Color::black), 0);
}

TEST(IntOfTest, Square) {
    EXPECT_EQ(intOf(Square::A1), 0);
    EXPECT_EQ(intOf(Square::H8), 63);
    EXPECT_EQ(intOf(Square::E4), 28);
}

TEST(IntOfTest, PieceType) {
    EXPECT_EQ(intOf(PieceType::none),   0);
    EXPECT_EQ(intOf(PieceType::pawn),   1);
    EXPECT_EQ(intOf(PieceType::knight), 2);
    EXPECT_EQ(intOf(PieceType::bishop), 3);
    EXPECT_EQ(intOf(PieceType::rook),   4);
    EXPECT_EQ(intOf(PieceType::queen),  5);
    EXPECT_EQ(intOf(PieceType::king),   6);
}

// ============================================================
// Move construction
// ============================================================

TEST(MoveTest, BasicMove) {
    Move m(Square::E2, Square::E4);
    EXPECT_EQ(m.from, Square::E2);
    EXPECT_EQ(m.to,   Square::E4);
    EXPECT_FALSE(m.promotion.has_value());
    EXPECT_FALSE(m.drop.has_value());
}

TEST(MoveTest, PromotionMove) {
    Move m(Square::E7, Square::E8, PieceType::queen);
    EXPECT_EQ(m.from, Square::E7);
    EXPECT_EQ(m.to,   Square::E8);
    ASSERT_TRUE(m.promotion.has_value());
    EXPECT_EQ(*m.promotion, PieceType::queen);
}

// ============================================================
// _sliding_attacks / _step_attacks
// ============================================================

TEST(AttackTableTest, SlidingAttacksRookEmpty) {
    // Rook on E4 with no blockers along rank
    Bitboard attacks = _sliding_attacks(Square::E4, BB_EMPTY, {-1, 1});
    EXPECT_TRUE(attacks & BB_SQUARES(Square::A4));
    EXPECT_TRUE(attacks & BB_SQUARES(Square::H4));
    EXPECT_FALSE(attacks & BB_SQUARES(Square::E4));
}

TEST(AttackTableTest, SlidingAttacksWithBlocker) {
    // Rook on E4, blocker on G4 — should not reach H4
    Bitboard occ = BB_SQUARES(Square::G4);
    Bitboard attacks = _sliding_attacks(Square::E4, occ, {-1, 1});
    EXPECT_TRUE(attacks & BB_SQUARES(Square::G4));   // captures blocker
    EXPECT_FALSE(attacks & BB_SQUARES(Square::H4));  // blocked
}

TEST(AttackTableTest, StepAttacksKnight) {
    Bitboard attacks = _step_attacks(Square::E4, {-17, -15, -10, -6, 6, 10, 15, 17});
    EXPECT_EQ(std::popcount(attacks), 8);
}

// ============================================================
// BoardState
// ============================================================

class BoardStateTest : public ::testing::Test {
protected:
    Board board;  // default starting position
};

TEST_F(BoardStateTest, SnapshotPreservesOccupancy) {
    BoardState state(board);
    EXPECT_EQ(state.occupied, board.occupied);
    EXPECT_EQ(state.occupied_w, board.occupied_co[intOf(Color::white)]);
    EXPECT_EQ(state.occupied_b, board.occupied_co[intOf(Color::black)]);
}

TEST_F(BoardStateTest, SnapshotPreservesPieces) {
    BoardState state(board);
    EXPECT_EQ(state.pawns,   board.pawns);
    EXPECT_EQ(state.knights, board.knights);
    EXPECT_EQ(state.bishops, board.bishops);
    EXPECT_EQ(state.rooks,   board.rooks);
    EXPECT_EQ(state.queens,  board.queens);
    EXPECT_EQ(state.kings,   board.kings);
}

TEST_F(BoardStateTest, SnapshotPreservesGameState) {
    BoardState state(board);
    EXPECT_EQ(state.turn,            board.turn);
    EXPECT_EQ(state.castling_rights, board.castling_rights);
    EXPECT_EQ(state.ep_square,       board.ep_square);
    EXPECT_EQ(state.halfmove_clock,  board.halfmove_clock);
    EXPECT_EQ(state.fullmove_number, board.fullmove_number);
}

TEST_F(BoardStateTest, RestoreAfterPush) {
    BoardState state(board);

    // After pushing e2e4, restoring the state should return to the original position
    board.push_uci("e2e4");
    state.restore(board);

    EXPECT_EQ(board.turn, Color::white);
    EXPECT_EQ(board.piece_type_at(Square::E2), std::optional<PieceType>(PieceType::pawn));
    EXPECT_FALSE(board.piece_type_at(Square::E4).has_value());
}

TEST_F(BoardStateTest, RestorePreservesHalfmoveClock) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    BoardState state(board);
    int hmc = board.halfmove_clock;

    board.push_uci("g1f3");
    EXPECT_EQ(board.halfmove_clock, hmc + 1);

    state.restore(board);
    EXPECT_EQ(board.halfmove_clock, hmc);
}

TEST_F(BoardStateTest, RestorePreservesFullmoveNumber) {
    BoardState state(board);
    board.push_uci("e2e4");
    board.push_uci("e7e5");  // fullmove_number → 2
    EXPECT_EQ(board.fullmove_number, 2);

    state.restore(board);
    EXPECT_EQ(board.fullmove_number, 1);
}

TEST_F(BoardStateTest, RestorePreservesEpSquare) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    board.push_uci("e4e5");
    board.push_uci("f7f5");  // ep_square = f6

    BoardState state(board);
    ASSERT_TRUE(board.ep_square.has_value());
    Square ep = *board.ep_square;

    board.push_uci("e5f6");  // execute en passant
    state.restore(board);

    EXPECT_EQ(board.ep_square, std::optional<Square>(ep));
}

TEST_F(BoardStateTest, SnapshotFromNonStartingFen) {
    Board b("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    BoardState state(b);
    EXPECT_EQ(state.castling_rights, b.castling_rights);
    EXPECT_EQ(state.turn, Color::white);
}

// ============================================================
// Board — constructor / reset / clear
// ============================================================

class BoardTest : public ::testing::Test {
protected:
    Board board;  // default starting position
};

TEST_F(BoardTest, DefaultFenIsStartingPosition) {
    EXPECT_EQ(*board.king(Color::white), Square::E1);
    EXPECT_EQ(*board.king(Color::black), Square::E8);
    EXPECT_EQ(board.turn, Color::white);
    EXPECT_EQ(board.fullmove_number, 1);
    EXPECT_EQ(board.halfmove_clock,  0);
}

TEST_F(BoardTest, NullOptFenCreatesEmptyBoard) {
    Board b(std::nullopt);
    EXPECT_EQ(b.occupied, BB_EMPTY);
    EXPECT_FALSE(b.king(Color::white).has_value());
    EXPECT_EQ(b.castling_rights, BB_EMPTY);
}

TEST_F(BoardTest, CustomFenParsed) {
    Board b("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(b.turn, Color::black);
    ASSERT_TRUE(b.ep_square.has_value());
    EXPECT_EQ(*b.ep_square, Square::E3);
    EXPECT_TRUE(b.piece_type_at(Square::E4).has_value());
}

TEST_F(BoardTest, ResetRestoresStartingPosition) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    board.reset();

    EXPECT_EQ(board.turn, Color::white);
    EXPECT_EQ(board.fullmove_number, 1);
    EXPECT_EQ(board.halfmove_clock,  0);
    EXPECT_EQ(*board.king(Color::white), Square::E1);
    EXPECT_TRUE(board.move_stack.empty());
}

TEST_F(BoardTest, ClearEmptiesEverything) {
    board.clear();
    EXPECT_EQ(board.occupied, BB_EMPTY);
    EXPECT_EQ(board.castling_rights, BB_EMPTY);
    EXPECT_FALSE(board.ep_square.has_value());
    EXPECT_TRUE(board.move_stack.empty());
}

TEST_F(BoardTest, EmptyFactory) {
    Board b = Board::empty();
    EXPECT_EQ(b.occupied, BB_EMPTY);
}

TEST_F(BoardTest, ClearStackClearsHistory) {
    board.push_uci("e2e4");
    board.clear_stack();
    EXPECT_TRUE(board.move_stack.empty());
}

// ============================================================
// Board — Ply / Root
// ============================================================

TEST_F(BoardTest, PlyInitialIsZero) {
    EXPECT_EQ(board.ply(), 0);
}

TEST_F(BoardTest, PlyIncreasesEachHalfMove) {
    board.push_uci("e2e4");
    EXPECT_EQ(board.ply(), 1);
    board.push_uci("e7e5");
    EXPECT_EQ(board.ply(), 2);
}

TEST_F(BoardTest, RootReturnsInitialPosition) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    Board root = board.root();
    EXPECT_EQ(root.turn, Color::white);
    EXPECT_EQ(root.fullmove_number, 1);
    EXPECT_FALSE(root.piece_type_at(Square::E4).has_value());
}

// ============================================================
// Board — Push / Pop / Peek
// ============================================================

TEST_F(BoardTest, PushAndPopMove) {
    board.push_uci("e2e4");
    EXPECT_EQ(*board.piece_type_at(Square::E4), PieceType::pawn);
    EXPECT_FALSE(board.piece_type_at(Square::E2).has_value());

    Move m = board.pop();
    EXPECT_EQ(m.from, Square::E2);
    EXPECT_EQ(m.to,   Square::E4);
    EXPECT_TRUE(board.piece_type_at(Square::E2).has_value());
    EXPECT_FALSE(board.piece_type_at(Square::E4).has_value());
}

TEST_F(BoardTest, PeekReturnsLastMove) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    Move m = board.peek();
    EXPECT_EQ(m.from, Square::E7);
    EXPECT_EQ(m.to,   Square::E5);
}

TEST_F(BoardTest, PushSwitchesTurn) {
    EXPECT_EQ(board.turn, Color::white);
    board.push_uci("e2e4");
    EXPECT_EQ(board.turn, Color::black);
    board.push_uci("e7e5");
    EXPECT_EQ(board.turn, Color::white);
}

TEST_F(BoardTest, PushUpdatesFullmoveNumber) {
    EXPECT_EQ(board.fullmove_number, 1);
    board.push_uci("e2e4");
    EXPECT_EQ(board.fullmove_number, 1);
    board.push_uci("e7e5");
    EXPECT_EQ(board.fullmove_number, 2);
}

TEST_F(BoardTest, PushCaptureRemovesOpponentPiece) {
    // Scholar's mate setup: white bishop captures f7
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    board.push_uci("f1c4");
    board.push_uci("b8c6");
    board.push_uci("c4f7");

    EXPECT_EQ(*board.piece_type_at(Square::F7), PieceType::bishop);
    EXPECT_EQ(*board.color_at(Square::F7), Color::white);
}

TEST_F(BoardTest, PushEnPassantCapture) {
    board.push_uci("e2e4");
    board.push_uci("a7a6");
    board.push_uci("e4e5");
    board.push_uci("f7f5");  // → ep_square = f6
    ASSERT_TRUE(board.ep_square.has_value());
    EXPECT_EQ(*board.ep_square, Square::F6);

    board.push_uci("e5f6");  // en passant capture
    EXPECT_FALSE(board.piece_type_at(Square::F5).has_value());  // captured black pawn
    EXPECT_EQ(*board.piece_type_at(Square::F6), PieceType::pawn);
}

TEST_F(BoardTest, PushPromotion) {
    Board b("8/P7/8/8/8/8/8/4K2k w - - 0 1");
    b.push_uci("a7a8q");
    EXPECT_EQ(*b.piece_type_at(Square::A8), PieceType::queen);
    EXPECT_EQ(*b.color_at(Square::A8), Color::white);
    EXPECT_FALSE(b.piece_type_at(Square::A7).has_value());
}

TEST_F(BoardTest, PushKingsideCastling) {
    Board b("r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    b.push_uci("e1g1");
    EXPECT_EQ(*b.piece_type_at(Square::G1), PieceType::king);
    EXPECT_EQ(*b.piece_type_at(Square::F1), PieceType::rook);
    EXPECT_FALSE(b.piece_type_at(Square::E1).has_value());
    EXPECT_FALSE(b.piece_type_at(Square::H1).has_value());
}

TEST_F(BoardTest, PushQueensideCastling) {
    Board b("r3kbnr/pppqpppp/2np4/4b3/3PP3/2NB1N2/PPP1BPPP/R3K2R w KQkq - 0 7");
    b.push_uci("e1c1");
    EXPECT_EQ(*b.piece_type_at(Square::C1), PieceType::king);
    EXPECT_EQ(*b.piece_type_at(Square::D1), PieceType::rook);
    EXPECT_FALSE(b.piece_type_at(Square::E1).has_value());
    EXPECT_FALSE(b.piece_type_at(Square::A1).has_value());
}

TEST_F(BoardTest, PushZerosHalfmoveClockOnPawnMove) {
    board.push_uci("g1f3");  // piece move → halfmove incremented
    EXPECT_EQ(board.halfmove_clock, 1);
    board.push_uci("e7e5");  // pawn move → halfmove reset
    EXPECT_EQ(board.halfmove_clock, 0);
}

TEST_F(BoardTest, PushZerosHalfmoveClockOnCapture) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    board.push_uci("e4d5");  // capture → halfmove reset
    EXPECT_EQ(board.halfmove_clock, 0);
}

TEST_F(BoardTest, PopRestoresEpSquare) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    board.push_uci("e4e5");
    board.push_uci("f7f5");  // ep_square = f6
    board.pop();
    EXPECT_FALSE(board.ep_square.has_value());
}

TEST_F(BoardTest, MultiplePopRestoresPosition) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    board.push_uci("g1f3");
    board.pop(); board.pop(); board.pop();

    EXPECT_EQ(board.turn, Color::white);
    EXPECT_EQ(board.fullmove_number, 1);
    EXPECT_TRUE(board.move_stack.empty());
    EXPECT_FALSE(board.piece_type_at(Square::E4).has_value());
}

// ============================================================
// Board — legal move generation
// ============================================================

TEST_F(BoardTest, LegalMoveCountStartingPosition) {
    auto moves = board.generate_legal_moves();
    EXPECT_EQ(moves.size(), 20u);
}

TEST_F(BoardTest, LegalMovesAfterE4) {
    board.push_uci("e2e4");
    auto moves = board.generate_legal_moves();
    EXPECT_EQ(moves.size(), 20u);
}

TEST_F(BoardTest, LegalMovesKiwiPete) {
    // This position is well-known to have 48 legal moves
    Board b("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    auto moves = b.generate_legal_moves();
    EXPECT_EQ(moves.size(), 48u);
}

TEST_F(BoardTest, NoLegalMovesInCheckmate) {
    // Right after Fool's Mate
    Board b("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(b.generate_legal_moves().empty());
}

TEST_F(BoardTest, GenerateLegalEpValid) {
    board.push_uci("e2e4");
    board.push_uci("a7a6");
    board.push_uci("e4e5");
    board.push_uci("f7f5");
    auto ep_moves = board.generate_legal_ep();
    EXPECT_EQ(ep_moves.size(), 1u);
    EXPECT_EQ(ep_moves[0].to, Square::F6);
}

TEST_F(BoardTest, GenerateLegalCaptures) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    auto caps = board.generate_legal_captures();
    // There should be exactly one capture: e4xd5
    bool found = false;
    for (auto& m : caps)
        if (m.from == Square::E4 && m.to == Square::D5) { found = true; break; }
    EXPECT_TRUE(found);
}

TEST_F(BoardTest, GenerateCastlingMoves) {
    Board b("r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    auto castling = b.generate_castling_moves();
    // One kingside castling move should be available
    EXPECT_EQ(castling.size(), 1u);
    EXPECT_TRUE(b.is_kingside_castling(castling[0]));
}

// ============================================================
// Board — move validation
// ============================================================

TEST_F(BoardTest, IsLegalValidMove) {
    EXPECT_TRUE(board.is_legal(Move(Square::E2, Square::E4)));
}

TEST_F(BoardTest, IsLegalInvalidMove) {
    EXPECT_FALSE(board.is_legal(Move(Square::E2, Square::E5)));
}

TEST_F(BoardTest, IsPseudoLegalPawnPush) {
    EXPECT_TRUE(board.is_pseudo_legal(Move(Square::E2, Square::E4)));
}

TEST_F(BoardTest, IsIntoCheckDetected) {
    // Moving a pinned piece exposes the own king to check
    Board b("8/8/8/8/8/3r4/3P4/3K4 w - - 0 1");
    // The D2 pawn shields the D1 king from the D3 rook — cannot move
    EXPECT_TRUE(b.is_into_check(Move(Square::D2, Square::D3)));
    EXPECT_TRUE(b.is_into_check(Move(Square::D2, Square::D4)));
}

TEST_F(BoardTest, FindMovePromotion) {
    Board b("8/P7/8/8/8/8/8/4K2k w - - 0 1");
    Move m = b.find_move(Square::A7, Square::A8);
    // Default promotion piece is queen
    ASSERT_TRUE(m.promotion.has_value());
    EXPECT_EQ(*m.promotion, PieceType::queen);
}

TEST_F(BoardTest, FindMoveIllegalThrows) {
    EXPECT_THROW(board.find_move(Square::E2, Square::E5), std::invalid_argument);
}

// ============================================================
// Board — check / checkmate / stalemate
// ============================================================

TEST_F(BoardTest, IsCheckFalseAtStart) {
    EXPECT_FALSE(board.is_check());
}

TEST_F(BoardTest, IsCheckTrueAfterCheck) {
    Board b("rnbqkbnr/pppp1ppp/8/4p3/5PP1/8/PPPPP2P/RNBQKBNR b KQkq f3 0 2");
    b.push_uci("d8h4");  // queen delivers check
    EXPECT_TRUE(b.is_check());
}

TEST_F(BoardTest, GivesCheckDetected) {
    Board b("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    // Qh5 threatens f7 but does not give an immediate check
    EXPECT_FALSE(b.gives_check(Move(Square::D1, Square::H5)));
}

TEST_F(BoardTest, IsCheckmateFoolsMate) {
    Board b("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(b.is_checkmate());
}

TEST_F(BoardTest, IsCheckmateNotAtStart) {
    EXPECT_FALSE(board.is_checkmate());
}

TEST_F(BoardTest, IsStalemateDetected) {
    // Classic stalemate position
    Board b("k7/8/1Q6/8/8/8/8/7K b - - 0 1");
    EXPECT_TRUE(b.is_stalemate());
}

TEST_F(BoardTest, IsStalemateNotAtStart) {
    EXPECT_FALSE(board.is_stalemate());
}

// ============================================================
// Board — draw conditions
// ============================================================

TEST_F(BoardTest, InsufficientMaterialKingVsKing) {
    Board b("8/8/4k3/8/8/4K3/8/8 w - - 0 1");
    EXPECT_TRUE(b.is_insufficient_material());
}

TEST_F(BoardTest, InsufficientMaterialKingAndBishopVsKing) {
    Board b("8/8/4k3/8/8/4K3/8/5B2 w - - 0 1");
    EXPECT_TRUE(b.is_insufficient_material());
}

TEST_F(BoardTest, NotInsufficientMaterialWithPawn) {
    Board b("8/P7/4k3/8/8/4K3/8/8 w - - 0 1");
    EXPECT_FALSE(b.is_insufficient_material());
}

TEST_F(BoardTest, IsSeventyFiveMoves) {
    Board b("8/8/4k3/8/8/4K3/8/8 w - - 150 1");
    // Kings are placed to ensure at least one legal move exists
    // 150 or more half-moves → automatic draw
    EXPECT_TRUE(b.is_seventyfive_moves());
}

TEST_F(BoardTest, IsFiftyMoves) {
    Board b("8/8/4k3/8/8/4K3/8/8 w - - 100 1");
    EXPECT_TRUE(b.is_fifty_moves());
}

TEST_F(BoardTest, IsRepetitionFalseAtStart) {
    EXPECT_FALSE(board.is_repetition());
}

TEST_F(BoardTest, IsRepetitionAfterThreefold) {
    // Repeat the same position three times
    for (int i = 0; i < 2; ++i) {
        board.push_uci("g1f3");
        board.push_uci("g8f6");
        board.push_uci("f3g1");
        board.push_uci("f6g8");
    }
    EXPECT_TRUE(board.is_repetition(3));
}

TEST_F(BoardTest, CanClaimThreefoldRepetition) {
    for (int i = 0; i < 2; ++i) {
        board.push_uci("g1f3");
        board.push_uci("g8f6");
        board.push_uci("f3g1");
        board.push_uci("f6g8");
    }
    EXPECT_TRUE(board.can_claim_threefold_repetition());
}

// ============================================================
// Board — game over / result
// ============================================================

TEST_F(BoardTest, IsGameOverFalseAtStart) {
    EXPECT_FALSE(board.is_game_over());
}

TEST_F(BoardTest, IsGameOverCheckmateTrue) {
    Board b("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(b.is_game_over());
}

TEST_F(BoardTest, OutcomeCheckmateWinner) {
    Board b("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    auto o = b.outcome();
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(o->termination, Termination::CHECKMATE);
    EXPECT_EQ(o->winner, Color::black);
}

TEST_F(BoardTest, OutcomeStalemateNoWinner) {
    Board b("k7/8/1Q6/8/8/8/8/7K b - - 0 1");
    auto o = b.outcome();
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(o->termination, Termination::STALEMATE);
}

TEST_F(BoardTest, ResultStringCheckmate) {
    Board b("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_EQ(b.result(), "0-1");
}

TEST_F(BoardTest, ResultStringOngoing) {
    EXPECT_EQ(board.result(), "*");
}

// ============================================================
// Board — move classification
// ============================================================

TEST_F(BoardTest, IsCaptureTrue) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    EXPECT_TRUE(board.is_capture(Move(Square::E4, Square::D5)));
}

TEST_F(BoardTest, IsCaptureFalse) {
    EXPECT_FALSE(board.is_capture(Move(Square::E2, Square::E4)));
}

TEST_F(BoardTest, IsEnPassantTrue) {
    board.push_uci("e2e4");
    board.push_uci("a7a6");
    board.push_uci("e4e5");
    board.push_uci("f7f5");
    EXPECT_TRUE(board.is_en_passant(Move(Square::E5, Square::F6)));
}

TEST_F(BoardTest, IsEnPassantFalse) {
    EXPECT_FALSE(board.is_en_passant(Move(Square::E2, Square::E4)));
}

TEST_F(BoardTest, IsZeroingPawnMove) {
    EXPECT_TRUE(board.is_zeroing(Move(Square::E2, Square::E4)));
}

TEST_F(BoardTest, IsZeroingCapture) {
    board.push_uci("e2e4");
    board.push_uci("d7d5");
    EXPECT_TRUE(board.is_zeroing(Move(Square::E4, Square::D5)));
}

TEST_F(BoardTest, IsZeroingFalseForPieceMove) {
    EXPECT_FALSE(board.is_zeroing(Move(Square::G1, Square::F3)));
}

TEST_F(BoardTest, IsCastlingKingside) {
    Board b("r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    EXPECT_TRUE(b.is_castling(Move(Square::E1, Square::G1)));
    EXPECT_TRUE(b.is_kingside_castling(Move(Square::E1, Square::G1)));
    EXPECT_FALSE(b.is_queenside_castling(Move(Square::E1, Square::G1)));
}

TEST_F(BoardTest, IsCastlingQueenside) {
    Board b("r3kbnr/pppqpppp/2np4/4b3/3PP3/2NB1N2/PPP1BPPP/R3K2R w KQkq - 0 7");
    EXPECT_TRUE(b.is_castling(Move(Square::E1, Square::C1)));
    EXPECT_TRUE(b.is_queenside_castling(Move(Square::E1, Square::C1)));
    EXPECT_FALSE(b.is_kingside_castling(Move(Square::E1, Square::C1)));
}

TEST_F(BoardTest, IsIrreversiblePawnMove) {
    EXPECT_TRUE(board.is_irreversible(Move(Square::E2, Square::E4)));
}

// ============================================================
// Board — castling rights
// ============================================================

TEST_F(BoardTest, HasCastlingRightsAtStart) {
    EXPECT_TRUE(board.has_castling_rights(Color::white));
    EXPECT_TRUE(board.has_castling_rights(Color::black));
}

TEST_F(BoardTest, HasKingsideCastlingRights) {
    EXPECT_TRUE(board.has_kingside_castling_rights(Color::white));
    EXPECT_TRUE(board.has_kingside_castling_rights(Color::black));
}

TEST_F(BoardTest, HasQueensideCastlingRights) {
    EXPECT_TRUE(board.has_queenside_castling_rights(Color::white));
    EXPECT_TRUE(board.has_queenside_castling_rights(Color::black));
}

TEST_F(BoardTest, CastlingRightsLostAfterKingMove) {
    Board b("r3kbnr/pppqpppp/2np4/4b3/3PP3/2NB1N2/PPP1BPPP/R3K2R w KQkq - 0 7");
    b.push_uci("e1f1");  // king moves → castling rights forfeited
    EXPECT_FALSE(b.has_castling_rights(Color::white));
}

TEST_F(BoardTest, CastlingRightsLostAfterRookMove) {
    board.push_uci("g1f3");
    board.push_uci("g8f6");
    board.push_uci("h1g1");  // h1 rook moves → kingside right forfeited
    EXPECT_FALSE(board.has_kingside_castling_rights(Color::white));
    EXPECT_TRUE(board.has_queenside_castling_rights(Color::white));
}

TEST_F(BoardTest, CleanCastlingRightsAtStart) {
    Bitboard cr = board.clean_castling_rights();
    EXPECT_TRUE(cr & BB_A1);
    EXPECT_TRUE(cr & BB_H1);
    EXPECT_TRUE(cr & BB_A8);
    EXPECT_TRUE(cr & BB_H8);
}

// ============================================================
// Board — FEN / SAN / UCI
// ============================================================

TEST_F(BoardTest, FenRoundTrip) {
    std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board b(fen);
    EXPECT_EQ(b.fen(), fen);
}

TEST_F(BoardTest, SetFenUpdatesTurn) {
    board.set_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(board.turn, Color::black);
}

TEST_F(BoardTest, SanE4) {
    Move m(Square::E2, Square::E4);
    EXPECT_EQ(board.san(m), "e4");
}

TEST_F(BoardTest, SanKnightF3) {
    Move m(Square::G1, Square::F3);
    EXPECT_EQ(board.san(m), "Nf3");
}

TEST_F(BoardTest, ParseSanAndPush) {
    Move m = board.parse_san("e4");
    EXPECT_EQ(m.from, Square::E2);
    EXPECT_EQ(m.to,   Square::E4);
}

TEST_F(BoardTest, ParseSanCastling) {
    Board b("r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    Move m = b.parse_san("O-O");
    EXPECT_TRUE(b.is_kingside_castling(m));
}

TEST_F(BoardTest, ParseSanCheck) {
    board.push_uci("e2e4");
    board.push_uci("e7e5");
    board.push_uci("f2f4");
    board.push_uci("d8h4");  // Qh4+ checks white king on e1
    EXPECT_TRUE(board.is_check());
}

TEST_F(BoardTest, UciRoundTrip) {
    Move m(Square::E2, Square::E4);
    std::string uci_str = board.uci(m);
    EXPECT_EQ(uci_str, "e2e4");
}

TEST_F(BoardTest, ParseUciAndPush) {
    Move m = board.parse_uci("e2e4");
    EXPECT_EQ(m.from, Square::E2);
    EXPECT_EQ(m.to,   Square::E4);
}

TEST_F(BoardTest, ParseUciPromotion) {
    Board b("8/P7/8/8/8/8/8/4K2k w - - 0 1");
    Move m = b.parse_uci("a7a8q");
    ASSERT_TRUE(m.promotion.has_value());
    EXPECT_EQ(*m.promotion, PieceType::queen);
}

TEST_F(BoardTest, ParseUciIllegalThrows) {
    EXPECT_THROW(board.parse_uci("e2e5"), std::invalid_argument);
}

TEST_F(BoardTest, LanLongNotation) {
    Move m(Square::G1, Square::F3);
    EXPECT_EQ(board.lan(m), "Ng1-f3");
}

TEST_F(BoardTest, EpdRoundTrip) {
    std::string epd = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    EXPECT_EQ(board.epd(), epd);
}

// ============================================================
// Board — validity check
// ============================================================

TEST_F(BoardTest, IsValidAtStart) {
    EXPECT_TRUE(board.is_valid());
}

TEST_F(BoardTest, IsValidFalseWithNoPieces) {
    Board b(std::nullopt);
    EXPECT_FALSE(b.is_valid());
}

TEST_F(BoardTest, StatusValidAtStart) {
    EXPECT_EQ(board.get_status(), status::VALID);
}

// ============================================================
// Board — copy / mirror / transform
// ============================================================

TEST_F(BoardTest, CopyIsIndependent) {
    Board copy = board.copy();
    copy.push_uci("e2e4");
    EXPECT_FALSE(board.piece_type_at(Square::E4).has_value());
    EXPECT_EQ(board.turn, Color::white);
}

TEST_F(BoardTest, CopyWithoutStack) {
    board.push_uci("e2e4");
    Board copy = board.copy(false);
    EXPECT_TRUE(copy.move_stack.empty());
    EXPECT_EQ(*copy.piece_type_at(Square::E4), PieceType::pawn);
}

TEST_F(BoardTest, MirrorSwapsTurn) {
    Board mirrored = board.mirror();
    EXPECT_EQ(mirrored.turn, Color::black);
}

TEST_F(BoardTest, MirrorSwapsPieces) {
    board.push_uci("e2e4");
    Board mirrored = board.mirror();
    // Original e4 (white pawn) should map to e5 (black pawn) after mirroring
    EXPECT_EQ(*mirrored.piece_type_at(Square::E5), PieceType::pawn);
    EXPECT_EQ(*mirrored.color_at(Square::E5), Color::black);
}

TEST_F(BoardTest, EqualityOperator) {
    Board b1, b2;
    EXPECT_TRUE(b1 == b2);
    b1.push_uci("e2e4");
    EXPECT_FALSE(b1 == b2);
    b1.pop();
    EXPECT_TRUE(b1 == b2);
}

// ============================================================
// Board — en passant validity
// ============================================================

TEST_F(BoardTest, HasLegalEnPassant) {
    board.push_uci("e2e4");
    board.push_uci("a7a6");
    board.push_uci("e4e5");
    board.push_uci("f7f5");
    EXPECT_TRUE(board.has_legal_en_passant());
}

TEST_F(BoardTest, NoEnPassantAtStart) {
    EXPECT_FALSE(board.has_legal_en_passant());
    EXPECT_FALSE(board.has_pseudo_legal_en_passant());
}