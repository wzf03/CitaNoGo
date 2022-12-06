#include "board.h"

#include <iostream>
#include <vector>
#define FOREACHADJ(BLOCK)                 \
  {                                       \
    Position ADJOFFSET = -(MAX_SIZE + 1); \
    {BLOCK};                              \
    ADJOFFSET = -1;                       \
    {BLOCK};                              \
    ADJOFFSET = 1;                        \
    {BLOCK};                              \
    ADJOFFSET = MAX_SIZE + 1;             \
    {BLOCK};                              \
  };
namespace cita {

Board::Board() {
  for (int i = 0; i < MAX_ARR_SIZE; i++) {
    board_[i] = POINT_WALL;
  }
  for (int x = 0; x < MAX_SIZE; x++) {
    for (int y = 0; y < MAX_SIZE; y++) {
      board_[getPos(x, y)] = POINT_EMPTY;
    }
  }
}

bool Board::Place(const Position& pos, POINT stone_type) {
  if (board_[pos] != POINT_EMPTY || (board_[pos] & state_[pos]) != 0)
    return false;
  board_[pos] = stone_type;
  state_[pos] = STATE_FORBID;

  {
    BoardArr<bool> searched{};
    dfs_change_liberties(pos, dfs_count_liberties(pos, searched));
  }
  {
    FOREACHADJ(Position adj = pos + ADJOFFSET; BoardArr<bool> searched{};
               if (board_[adj] == getOpp(board_[pos])) dfs_change_liberties(
                   adj, dfs_count_liberties(adj, searched));)
  }
  {
    BoardArr<bool> searched{};
    dfs_affected_pos(pos, searched);
    FOREACHADJ(Position adj = pos + ADJOFFSET;
               if (board_[adj] == getOpp(board_[pos])) {
                 dfs_affected_pos(adj, searched);
               })
  }

  return true;
}

BoardGameArr<Position> Board::GetValidPlace(POINT stone_type, int& len) const {
  BoardGameArr<Position> res{};
  int numres{0};
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Position pos = getPos(i, j);
      if ((state_[pos] & stone_type) == 0) {
        res[numres++] = pos;
      }
    }
  }
  len = numres;
  return res;
}

void Board::dfs_affected_pos(Position pos, BoardArr<bool>& searched) {
  searched[pos] = true;

  FOREACHADJ(
      Position adj = pos + ADJOFFSET; if (board_[adj] == POINT_EMPTY) {
        update_empty(adj);
      } else if (!searched[adj] && board_[pos] == board_[adj]) {
        dfs_affected_pos(adj, searched);
      })
}
int Board::dfs_count_liberties(Position pos, BoardArr<bool>& searched) {
  int cnt = 0;
  searched[pos] = true;
  FOREACHADJ(Position adj = pos + ADJOFFSET; if (!searched[adj]) {
    if (board_[adj] == board_[pos]) {
      cnt += dfs_count_liberties(adj, searched);
    } else if (board_[adj] == POINT_EMPTY) {
      cnt += 1;
      searched[adj] = true;
    }
  })
  return cnt;
}
void Board::dfs_change_liberties(Position pos, int liberties) {
  liberties_[pos] = liberties;
  FOREACHADJ(
      Position adj = pos + ADJOFFSET;
      if (!(board_[adj] != board_[pos] || liberties_[adj] == liberties)) {
        dfs_change_liberties(adj, liberties);
      })
}
void Board::update_empty(Position pos) {
  state_[pos] = STATE_ALLOW;

  bool is_eye = true;
  bool will_black_suicide = true, will_white_suicide = true;
  bool exist_black = false, exist_white = false;
  POINT eye_type = POINT_EMPTY;

  FOREACHADJ(
      Position adj = pos + ADJOFFSET; if (board_[adj] == POINT_EMPTY) {
        is_eye = false;
        will_black_suicide = false;
        will_white_suicide = false;
      } else if (is_eye && board_[adj] != POINT_WALL) {
        if (eye_type == POINT_EMPTY)
          eye_type = board_[adj];
        else if (eye_type == getOpp(board_[adj]))
          is_eye = false;
      }

      if (board_[adj] == POINT_WHITE) {
        exist_white = true;
        if (liberties_[adj] <= 1) {
          state_[pos] = STATE(state_[pos] | STATE_FORBID_BLACK);
        } else {
          will_white_suicide = false;
        }
      } if (board_[adj] == POINT_BLACK) {
        exist_black = true;
        if (liberties_[adj] <= 1) {
          state_[pos] = STATE(state_[pos] | STATE_FORBID_WHITE);
        } else {
          will_black_suicide = false;
        }
      })

  if ((is_eye && eye_type == POINT_BLACK) ||
      (will_white_suicide && exist_white)) {
    state_[pos] = STATE(state_[pos] | STATE_FORBID_WHITE);
  }
  if ((is_eye && eye_type == POINT_WHITE) ||
      (will_black_suicide && exist_black)) {
    state_[pos] = STATE(state_[pos] | STATE_FORBID_BLACK);
  }
}
}  // namespace cita
