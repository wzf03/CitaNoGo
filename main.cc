#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>

#include "board.h"

#ifndef SIMPLE_REACT
#include <nlohmann/json.hpp>
using nlohmann::json;
#endif

using namespace cita;
using std::cin;
using std::cout;
using std::endl;
using std::string;

constexpr double C = 0.5;
constexpr std::chrono::milliseconds TIME_LIMIT(950);

long long LOOPTIMES = 0;

std::random_device rd;
std::mt19937_64 re(rd());
std::uniform_int_distribution<int> dis(0);

//---------------------------- MCTS end ----------------------------------
struct Node {
  Position pos{};
  double ucb1{};
  int n{};
  double v{};
  bool full_expanded{false};
  bool terminal{false};
  POINT stone_type{};

  Node* parent{nullptr};

  BoardGameArr<Node*> child{};
  int num_child{};
  BoardGameArr<Position> can_place{};
  int num_can_place{};

  Node(Board& board, const Position& pos, Node* parent, POINT currentStone)
      : pos(pos), stone_type(currentStone), parent(parent) {
    if (pos != 0) board.Place(pos, currentStone);
    can_place = board.GetValidPlace(getOpp(currentStone), num_can_place);

    if (num_can_place == 0) {
      terminal = true;
    }
    std::shuffle(can_place.begin(), can_place.begin() + num_can_place, re);
  }
  ~Node() {
    for (auto i = child.begin(); i < child.begin() + num_child; i++) {
      delete *i;
    }
  }
  Node* expand(Board& board) {
    if (full_expanded == true) return nullptr;
    child[num_child] =
        new Node(board, can_place[num_child], this, getOpp(stone_type));
    num_child++;
    if (num_child == num_can_place) {
      full_expanded = true;
    }
    return child[num_child - 1];
  }
};

Node* best_child(Node* node) {
  auto result{*node->child.begin()};
  for (auto i{node->child.begin()}; i < node->child.begin() + node->num_child;
       i++) {
    auto current{*i};
    current->ucb1 = (current->v) / (current->n + 1) +
                    C * std::sqrt(std::log(node->n + 1) / (current->n + 1));
    if (current->ucb1 > result->ucb1) {
      result = current;
    } else if (current->ucb1 == result->ucb1 && dis(re) % 2 == 0) {
      result = current;
    }
  }
  return result;
}

Node* tree_policy(Board& board, Node* root) {
  Node* result{root};
  while (!result->terminal) {
    result->n++;
    if (result->full_expanded) {
      result = best_child(result);
      board.Place(result->pos, result->stone_type);
    } else {
      result = result->expand(board);
      break;
    }
  }
  result->n++;
  return result;
}

Position random_rollout_policy(const Board& board, POINT currentStone) {
  int len{};
  BoardGameArr<Position> canPlace{board.GetValidPlace(currentStone, len)};
  if (len == 0) {
    return 0;
  } else {
    return canPlace[dis(re) % len];
  }
}

Position greedy_rollout_policy(const Board& board, POINT currentStone) {
  int len{}, maxGain{-10000};
  Position result{};
  BoardGameArr<Position> canPlace{board.GetValidPlace(currentStone, len)};

  if (len == 0) {
    return 0;
  }

  for (int i = 0; i < len; i++) {
    Board current = board;
    current.Place(canPlace[i], currentStone);
    int currentGain = current.GetGain(currentStone);
    if (currentStone > maxGain) {
      maxGain = currentGain;
      result = canPlace[i];
    }
  }
  return result;
}

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

void backup(Node* node, double result, POINT currentStone) {
  while (node != nullptr) {
    node->v += node->stone_type == currentStone ? result : 1 - result;
    node = node->parent;
  }
}

Position MCTsearch(Board& board, Node* root, POINT currentStone) {
  auto begin{std::chrono::steady_clock::now()};
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                       begin) < TIME_LIMIT) {
    Board current{board};
    Node* leaf{tree_policy(current, root)};
    double value{rollout(current, getOpp(leaf->stone_type))};
    backup(leaf, value, getOpp(leaf->stone_type));

    LOOPTIMES++;
  }
  return best_child(root)->pos;
}
//---------------------------- MCTS end ----------------------------------

int main() {
  POINT current = POINT_BLACK;
  Board board{};
#ifdef SIMPLE_REACT
  int n{};
  cin >> n;
  int cnt{2 * n - 1};
#else
  json input = json::parse(cin);
  int cnt{static_cast<int>(input["requests"].size() * 2 - 1)};
#endif

  for (int i = 0; i < cnt; i++) {
#ifdef SIMPLE_REACT
    int x{}, y{};
    cin >> x >> y;
#else
    string target{i % 2 == 0 ? "requests" : "responses"};
    int x{input[target][i / 2]["x"]}, y{input[target][i / 2]["y"]};
#endif

    if (x == -1) continue;

    board.Place(getPos(x, y), current);

    current = getOpp(current);
  }

  Node root(board, 0, nullptr, getOpp(current));
  Position result{MCTsearch(board, &root, current)};

#ifdef SIMPLE_REACT
  cout << getX(result) << ' ' << getY(result) << endl;
  cout << "loop_times: " << LOOPTIMES << endl;
#else
  json response = {{"response", {{"x", getX(result)}, {"y", getY(result)}}},
                   {"debug", {{"loop_times", LOOPTIMES}}}};

  cout << response.dump() << endl;
#endif
}
