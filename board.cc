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
    state_[i] = STATE_FORBID;
  }
  for (int x = 0; x < MAX_SIZE; x++) {
    for (int y = 0; y < MAX_SIZE; y++) {
      int pos = getPos(x, y);
      board_[pos] = POINT_EMPTY;
      state_[pos] = STATE_ALLOW;
    }
  }
}

bool Board::Place(const Position &pos, POINT stoneType) {
  if (board_[pos] != POINT_EMPTY || (board_[pos] & state_[pos]) != 0)
    return false;
  board_[pos] = stoneType;
  state_[pos] = STATE_FORBID;

  // connect the adjacent chain which type == stoneType
  chain_next_[pos] = pos;
  chain_head_[pos] = pos;
  Position chainToConnect[5]{pos};
  int numChainToConnect{1};

  FOREACHADJ(Position adj = pos + ADJOFFSET; if (board_[adj] == stoneType) {
    bool alreadyIncluded = false;
    for (int i = 1; i < numChainToConnect; i++) {
      if (chain_head_[adj] == chainToConnect[i]) {
        alreadyIncluded = true;
        break;
      }
    }
    if (!alreadyIncluded) {
      chainToConnect[numChainToConnect++] = chain_head_[adj];
    }
  })

  int currentChain{0};
  Position cur{pos};
  while (currentChain < numChainToConnect) {
    while (chain_next_[cur] != chainToConnect[currentChain]) {
      cur = chain_next_[cur];
      chain_head_[cur] = pos;
    }
    chain_next_[cur] = chainToConnect[(++currentChain) % numChainToConnect];
    chain_head_[cur] = pos;
    cur = chain_next_[cur];
  }

  {
    BoardArr<bool> counted{};
    chain_count_and_change_liberties(pos, counted);
  }
  {
    Position chainChanged[4]{};
    int numChainChanged{0};
    FOREACHADJ(Position adj = pos + ADJOFFSET;
               if (board_[adj] == getOpp(board_[pos])) {
                 bool alreadyChanged{false};
                 for (int i = 0; i < numChainToConnect; i++) {
                   if (chain_head_[adj] == chainChanged[i]) {
                     alreadyChanged = true;
                     break;
                   }
                 }
                 if (!alreadyChanged) {
                   BoardArr<bool> searched{};
                   chainChanged[numChainChanged++] = chain_head_[adj];
                   chain_count_and_change_liberties(adj, searched);
                 }
               })
  }
  {
    BoardArr<bool> searched{};
    chain_affected_pos(pos, searched);

    Position chainSearched[4]{};
    int numChainSearched{0};
    FOREACHADJ(Position adj = pos + ADJOFFSET;
               if (board_[adj] == getOpp(board_[pos])) {
                 bool alreadySearched{false};
                 for (int i = 0; i < numChainToConnect; i++) {
                   if (chain_head_[adj] == chainSearched[i]) {
                     alreadySearched = true;
                     break;
                   }
                 }
                 if (!alreadySearched) {
                   chainSearched[numChainSearched++] = chain_head_[adj];
                   chain_affected_pos(adj, searched);
                 }
               })
  }

  return true;
}

BoardGameArr<Position> Board::GetValidPlace(POINT stoneType, int &len) const {
  BoardGameArr<Position> res{};
  int numres{0};
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Position pos = getPos(i, j);
      if ((state_[pos] & stoneType) == 0) {
        res[numres++] = pos;
      }
    }
  }
  len = numres;
  return res;
}

int Board::GetValidPlaceCount(POINT stoneType) const {
  int count{0};
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Position pos = getPos(i, j);
      if ((state_[pos] & stoneType) == 0) {
        count++;
      }
    }
  }
  return count;
}

int Board::GetBowlCount(POINT stoneType) const {
  int count{0};
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Position pos = getPos(i, j);
      if (board_[pos] != POINT_EMPTY && state_[pos] != STATE_ALLOW) continue;
      int wallOrOurStoneCount{}, emptyCount{};
      FOREACHADJ(
          Position adj = pos + ADJOFFSET;
          if (board_[adj] == POINT_WALL || board_[adj] == stoneType) {
            wallOrOurStoneCount++;
          } else if (board_[adj] == POINT_EMPTY) { emptyCount++; } else {
            continue;  // jump out of outer loop
          } if (emptyCount > 1) { continue; })
      if (wallOrOurStoneCount == 3 && emptyCount == 1) {
        count++;
      }
    }
  }
  return count;
}

void Board::chain_affected_pos(Position pos, BoardArr<bool> &searched) {
  Position cur{pos};
  do {
    FOREACHADJ(Position adj = cur + ADJOFFSET;
               if (!searched[adj] && board_[adj] == POINT_EMPTY) {
                 update_empty(adj);
               })
    cur = chain_next_[cur];
  } while (cur != pos);
}

void Board::chain_count_and_change_liberties(Position loc,
                                             BoardArr<bool> &counted) {
  Position cur{loc};
  int numLiberties{0};
  do {
    FOREACHADJ(Position adj = cur + ADJOFFSET;
               if (!counted[adj] && board_[adj] == POINT_EMPTY) {
                 numLiberties++;
                 counted[adj] = true;
               })
    cur = chain_next_[cur];
  } while (cur != loc);

  cur = loc;
  do {
    liberties_[cur] = numLiberties;
    cur = chain_next_[cur];
  } while (cur != loc);
}

void Board::update_empty(Position pos) {
  state_[pos] = STATE_ALLOW;

  bool isEye{true};
  bool willBlackSuicide{true}, willWhiteSuicide{true};
  bool existBlack{false}, existWhite{false};
  POINT eyeType{POINT_EMPTY};

  FOREACHADJ(
      Position adj = pos + ADJOFFSET; if (board_[adj] == POINT_EMPTY) {
        isEye = false;
        willBlackSuicide = false;
        willWhiteSuicide = false;
      } else if (isEye && board_[adj] != POINT_WALL) {
        if (eyeType == POINT_EMPTY)
          eyeType = board_[adj];
        else if (eyeType == getOpp(board_[adj]))
          isEye = false;
      }

      if (board_[adj] == POINT_WHITE) {
        existWhite = true;
        if (liberties_[adj] <= 1) {
          state_[pos] = STATE(state_[pos] | STATE_FORBID_BLACK);
        } else {
          willWhiteSuicide = false;
        }
      } if (board_[adj] == POINT_BLACK) {
        existBlack = true;
        if (liberties_[adj] <= 1) {
          state_[pos] = STATE(state_[pos] | STATE_FORBID_WHITE);
        } else {
          willBlackSuicide = false;
        }
      })

  if ((isEye && eyeType == POINT_BLACK) || (willWhiteSuicide && existWhite)) {
    state_[pos] = STATE(state_[pos] | STATE_FORBID_WHITE);
  }
  if ((isEye && eyeType == POINT_WHITE) || (willBlackSuicide && existBlack)) {
    state_[pos] = STATE(state_[pos] | STATE_FORBID_BLACK);
  }
}
}  // namespace cita
