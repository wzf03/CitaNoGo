#ifndef CITA_MCTS_h_
#define CITA_MCTS_h_
#include <chrono>
#include <string>

#include "board.h"

namespace cita {
constexpr double C = 1;                               // UCB公式中常数
constexpr double K = 500;                             // RAVE优化中常数
constexpr std::chrono::milliseconds TIME_LIMIT(950);  // 时间限制

struct Node {
  int n{},  // 当前node被访问次数
      m{};  // RAVE优化中用到的值，为当前node所下的位置在当前局面和后来的局面被下的总次数
  Position pos{};  // 当前node所下棋子的位置
  double ucb1{};  // 存储node当时状态的ucb1值，每次调用best_child函数都会改变
  double v{},                 // mcts算法中每次模拟后得到的价值和
      vrave{};                // rave优化中的每次模拟的价值和
  bool full_expanded{false};  // 当前node是否被完全拓展
  bool terminal{false};  // 当前node对应的游戏是否已经结束，即有一方无法下子
  POINT stone_type{};  // 落子类型

  Node* parent{nullptr};        // 父node
  BoardGameArr<Node*> child{};  // 子node
  int num_child{};              // 子node数量

  BoardGameArr<Position> can_place{};  // 下一步可以落子的位置
  int num_can_place{};  // 下一步可以落子的位置的数量

  Node(Board& board, const Position& pos, Node* parent, POINT currentStone);
  ~Node();

  // 随机拓展node并返回
  Node* expand(Board& board);
};

struct DebugInfo {
  int loop_times{};
  double win_rate{};
  std::string info();
};

// 进行MCTS模拟并返回结果
Position UctSearch(Board& board, Node* root, DebugInfo& debug);
}  // namespace cita
#endif