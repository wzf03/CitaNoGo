#ifndef CITA_BOARD_h_
#define CITA_BOARD_h_
#include <array>
namespace cita {
constexpr short MAX_SIZE = 9;
constexpr int MAX_ARR_SIZE = (MAX_SIZE + 1) * (MAX_SIZE + 2) + 1;
constexpr int MAX_GAME_SIZE = MAX_SIZE * MAX_SIZE;

template <typename T> using BoardArr = std::array<T, MAX_ARR_SIZE>;
template <typename T> using BoardGameArr = std::array<T, MAX_GAME_SIZE>;
enum POINT {
  POINT_EMPTY = 0b00,
  POINT_BLACK = 0b01,
  POINT_WHITE = 0b10,
  POINT_WALL = 0b11
};
enum STATE {
  STATE_ALLOW = 0b00,
  STATE_FORBID_BLACK = 0b01,
  STATE_FORBID_WHITE = 0b10,
  STATE_FORBID = 0b11
};

using Position = short;

inline POINT getOpp(const POINT &point) {
  return point == POINT_WHITE ? POINT_BLACK : POINT_WHITE;
}

inline Position getPos(int x, int y) {
  return (x + 1) + (y + 1) * (MAX_SIZE + 1);
}
inline int getX(Position pos) { return pos % (MAX_SIZE + 1) - 1; }
inline int getY(Position pos) { return pos / (MAX_SIZE + 1) - 1; }

class Board {
public:
  Board();
  bool Place(const Position &pos, POINT stoneType);
  BoardGameArr<Position> GetValidPlace(POINT stoneType, int &len) const;

private:
  BoardArr<POINT> board_{};
  BoardArr<STATE> state_{};
  BoardArr<int> liberties_{};
  BoardArr<Position> chain_next_{}; // Circular chain
  BoardArr<Position> chain_head_{};

  void chain_affected_pos(Position pos, BoardArr<bool> &searched);
  void chain_count_and_change_liberties(Position pos, BoardArr<bool> &counted);
  void update_empty(Position pos);
};

} // namespace cita
#endif
