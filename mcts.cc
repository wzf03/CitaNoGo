#include "mcts.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <random>
#include <string>

#include "board.h"

namespace cita {

// 随机数生成相关
std::random_device rd;
std::mt19937_64 re(rd());
std::uniform_int_distribution<int> dis(0);

Node::Node(Board& board, const Position& pos, Node* parent, POINT currentStone)
    : pos(pos), stone_type(currentStone), parent(parent) {
  // 在棋盘上落子并获取下一步可落子位置
  if (pos != 0) board.Place(pos, currentStone);
  can_place = board.GetValidPlace(getOpp(currentStone), num_can_place);

  // 判断是否终局
  if (num_can_place == 0) {
    terminal = true;
  }

  // 打乱can_place以便达成随机拓展的效果
  std::shuffle(can_place.begin(), can_place.begin() + num_can_place, re);
}

Node::~Node() {
  for (int i = 0; i < num_child; i++) {
    delete child[i];
  }
}

Node* Node::expand(Board& board) {
  if (full_expanded == true) return nullptr;
  child[num_child] =
      new Node(board, can_place[num_child], this, getOpp(stone_type));
  num_child++;
  if (num_child == num_can_place) {
    full_expanded = true;
  }
  return child[num_child - 1];
}

std::string DebugInfo::info() {
  return "loop_times: " + std::to_string(loop_times) +
         "; win_rate: " + std::to_string(win_rate);
}

// 返回ucb1值最大的子node
Node* best_child(Node* node) {
  Node* bestChild{node->child[0]};

  for (int i = 0; i < node->num_child; i++) {
    Node* currentChild{node->child[i]};

    // RAVE算法中alpha值，当前node被访问次数越大则越接近1
    double alpha = currentChild->n / (K + currentChild->n);

    // 经过RAVE优化的UCB1公式
    double logN = std::log(node->n);
    currentChild->ucb1 = alpha * currentChild->v / currentChild->n +
                         (1 - alpha) * currentChild->vrave / currentChild->m +
                         C * std::sqrt(logN / currentChild->n);

    if (currentChild->ucb1 > bestChild->ucb1) {
      bestChild = currentChild;
    } else if (currentChild->ucb1 == bestChild->ucb1 && dis(re) % 2 == 0) {
      // 实现如果有多个ucb值相同的子node时随机选取的效果
      bestChild = currentChild;
    }
  }
  return bestChild;
}

// 获取平均价值最大的子节点
Node* best_child_without_ucb(Node* node) {
  Node* bestChild{node->child[0]};

  for (int i = 0; i < node->num_child; i++) {
    Node* currentChild{node->child[i]};
    double alpha = currentChild->n / (K + currentChild->n);
    double logN = std::log(node->n);
    currentChild->ucb1 = alpha * currentChild->v / currentChild->n +
                         (1 - alpha) * currentChild->vrave / currentChild->m;

    if (currentChild->ucb1 > bestChild->ucb1) {
      bestChild = currentChild;
    } else if (currentChild->ucb1 == bestChild->ucb1 && dis(re) % 2 == 0) {
      bestChild = currentChild;
    }
  }
  return bestChild;
}

// 从root出发根据ucb值一直找到下一个拓展的node并返回
Node* tree_policy(Board& board, Node* root) {
  Node* cur{root};
  while (!cur->terminal) {
    cur->n++;
    if (cur->full_expanded) {
      cur = best_child(cur);
      board.Place(cur->pos, cur->stone_type);
    } else {
      cur = cur->expand(board);
      break;
    }
  }
  cur->n++;
  return cur;
}

// 随机落子
Position random_rollout_policy(const Board& board, POINT currentStone) {
  int len{};
  BoardGameArr<Position> canPlace{board.GetValidPlace(currentStone, len)};
  if (len == 0) {
    return 0;
  } else {
    return canPlace[dis(re) % len];
  }
}

// 随机模拟到终局
double rollout(Board& board, POINT currentStone) {
  POINT initStone{currentStone};
  Position p;
  while (p = random_rollout_policy(board, currentStone), p != 0) {
    // while (p = greedy_rollout_policy(board, currentStone), p != 0) {
    board.Place(p, currentStone);
    currentStone = getOpp(currentStone);
  }
  if (initStone == currentStone) {
    return 0.0;
  } else {
    return 1.0;
  }
}

// RAVE优化中的反向传播
void backup_rave(Node* node, double result, POINT currentStone,
                 Position action) {
  while (node != nullptr) {
    if (node->stone_type != currentStone) {
      for (int i = 0; i < node->num_child; i++) {
        if (node->child[i]->pos == action) {
          node->child[i]->m++;
          node->child[i]->vrave += result;
        }
      }
    }
    node = node->parent;
  }
}

// 反向传播
void backup(Node* node, double result, POINT currentStone) {
  while (node != nullptr) {
    double currentV = node->stone_type == currentStone ? result : 1 - result;
    node->v += currentV;
    backup_rave(node, currentV, node->stone_type, node->pos);
    node = node->parent;
  }
}

Position UctSearch(Board& board, Node* root, DebugInfo& debug) {
  auto begin{std::chrono::steady_clock::now()};
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                       begin) < TIME_LIMIT) {
    Board current{board};
    Node* leaf{tree_policy(current, root)};
    double value{rollout(current, getOpp(leaf->stone_type))};
    backup(leaf, value, getOpp(leaf->stone_type));

    debug.loop_times++;
  }
  Node* choice = best_child_without_ucb(root);

  // 最后一次算出的ucb1是不带平衡项的，可以反映算法以为的胜率
  debug.win_rate = choice->ucb1;

  return choice->pos;
}
}  // namespace cita