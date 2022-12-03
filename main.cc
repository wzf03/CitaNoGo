#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifndef SIMPLE_REACT
#include <nlohmann/json.hpp>
using nlohmann::json;
#endif

using std::array;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

constexpr double C = 1.0;
constexpr std::chrono::milliseconds TIME_LIMIT(970);

template <typename T>
using Board = array<array<T, 9>, 9>;

enum POINT { POINT_EMPTY = 0b00, POINT_BLACK = 0b01, POINT_WHITE = 0b10 };
enum STATE {
  STATE_ALLOW = 0b00,
  STATE_FORBID_BLACK = 0b01,
  STATE_FORBID_WHITE = 0b10,
  STATE_FORBID = 0b11
};
struct Coord {
  int x, y;
};

class SmartBoard {
 public:
  bool Place(const Coord& pos, POINT stone_type);
  vector<Coord> GetValidPlace(POINT stone_type) const;

 private:
  Board<POINT> board_{};
  Board<STATE> state_{};
  Board<int> liberties_{};

  void dfs_affected_pos(int x, int y, Board<bool>& searched);
  int dfs_count_liberties(int x, int y, Board<bool>& searched);
  void dfs_change_liberties(int x, int y, int liberties);
  void update_empty(int x, int y);
};

std::random_device random_device;
std::default_random_engine random_engine(random_device());

long long LOOPTIMES = 0;

inline POINT other_stone_type(const POINT& point) {
  return point == POINT_WHITE ? POINT_BLACK : POINT_WHITE;
}

inline bool in_board(int x, int y) {
  return x >= 0 && y >= 0 && x < 9 && y < 9;
}

inline int rand() {
  static std::uniform_int_distribution<int> d(1, 81);
  return d(random_engine);
}

// MCTS start
struct Node {
  Coord pos{};
  double ucb1{};
  int n{};
  double v{};
  bool full_expanded{false};
  bool terminal{false};
  POINT stone_type{};

  Node* parent{nullptr};
  vector<Node*> child{};
  vector<Coord> can_place{};

  Node(SmartBoard& board, const Coord& pos, Node* parent, POINT current_stone);
  ~Node();
  Node* expand(SmartBoard& board);
};

Coord MCTsearch(SmartBoard& board, Node* root, POINT current_stone);
Node* tree_policy(SmartBoard& board, Node* root);
double rollout(SmartBoard& board, POINT current_stone);
Coord rollout_policy(const SmartBoard& board, POINT current_stone);
void backup(Node* node, double result, POINT current_stone);
Node* best_child(Node* node);
// MCTS end

int main() {
  POINT current = POINT_BLACK;
  SmartBoard board{};
#ifdef SIMPLE_REACT
  int n;
  cin >> n;
  int cnt = 2 * n - 1;
#else
  json input = json::parse(cin);
  int cnt = static_cast<int>(input["requests"].size() * 2 - 1);
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

    board.Place(Coord{x, y}, current);

    current = other_stone_type(current);
  }

  Node root(board, Coord{-1, -1}, nullptr, other_stone_type(current));
  Coord result = MCTsearch(board, &root, current);

#ifdef SIMPLE_REACT
  cout << result.x << ' ' << result.y << endl;
  cout << "loop_times: " << LOOPTIMES << endl;
#else
  json response = {{"response", {{"x", result.x}, {"y", result.y}}},
                   {"debug", {{"loop_times", LOOPTIMES}}}};

  cout << response.dump() << endl;
#endif
}

bool SmartBoard::Place(const Coord& pos, POINT stone_type) {
  const int &x = pos.x, y = pos.y;
  if ((board_[x][y] & state_[x][y]) != 0) return false;
  board_[x][y] = stone_type;
  state_[x][y] = STATE_FORBID;

  {
    Board<bool> searched{};
    dfs_change_liberties(x, y, dfs_count_liberties(x, y, searched));
  }
  {
    for (int i = 0; i < 4; i++) {
      int nx = x + ((i + 1) % 2) * (i - 1);
      int ny = y + (i % 2) * (i - 2);
      if (!in_board(nx, ny)) continue;
      Board<bool> searched{};
      dfs_change_liberties(nx, ny, dfs_count_liberties(nx, ny, searched));
    }
  }
  {
    Board<bool> searched{};
    dfs_affected_pos(x, y, searched);

    for (int i = 0; i < 4; i++) {
      int nx = x + ((i + 1) % 2) * (i - 1);
      int ny = y + (i % 2) * (i - 2);
      if (!in_board(nx, ny)) continue;
      if (board_[nx][ny] == other_stone_type(board_[x][y])) {
        dfs_affected_pos(nx, ny, searched);
      }
    }
  }

  return true;
}

vector<Coord> SmartBoard::GetValidPlace(POINT stone_type) const {
  vector<Coord> res{};
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if ((state_[i][j] & stone_type) == 0) {
        res.emplace_back(Coord{i, j});
      }
    }
  }
  return res;
}

void SmartBoard::dfs_affected_pos(int x, int y, Board<bool>& searched) {
  searched[x][y] = true;

  for (int i = 0; i < 4; i++) {
    int nx = x + ((i + 1) % 2) * (i - 1);
    int ny = y + (i % 2) * (i - 2);
    if (!in_board(nx, ny)) continue;
    if (board_[nx][ny] == POINT_EMPTY) {
      update_empty(nx, ny);
    } else if (!searched[nx][ny] && board_[x][y] == board_[nx][ny]) {
      dfs_affected_pos(nx, ny, searched);
    }
  }
}
int SmartBoard::dfs_count_liberties(int x, int y, Board<bool>& searched) {
  int cnt = 0;
  searched[x][y] = true;
  for (int i = 0; i < 4; i++) {
    int nx = x + ((i + 1) % 2) * (i - 1);
    int ny = y + (i % 2) * (i - 2);
    if (!in_board(nx, ny)) continue;
    if (!searched[nx][ny]) {
      if (board_[nx][ny] == board_[x][y]) {
        cnt += dfs_count_liberties(nx, ny, searched);
      } else if (board_[nx][ny] == POINT_EMPTY) {
        cnt += 1;
        searched[nx][ny] = true;
      }
    }
  }
  return cnt;
}
void SmartBoard::dfs_change_liberties(int x, int y, int liberties) {
  liberties_[x][y] = liberties;
  for (int i = 0; i < 4; i++) {
    int nx = x + ((i + 1) % 2) * (i - 1);
    int ny = y + (i % 2) * (i - 2);
    if (!in_board(nx, ny) || board_[nx][ny] != board_[x][y] ||
        liberties_[nx][ny] == liberties)
      continue;
    dfs_change_liberties(nx, ny, liberties);
  }
}
void SmartBoard::update_empty(int x, int y) {
  state_[x][y] = STATE_ALLOW;

  bool is_eye = true;
  bool will_black_suicide = true, will_white_suicide = true;
  bool exist_black = false, exist_white = false;
  POINT eye_type = POINT_EMPTY;

  for (int i = 0; i < 4; i++) {
    int nx = x + ((i + 1) % 2) * (i - 1);
    int ny = y + (i % 2) * (i - 2);
    if (!in_board(nx, ny)) continue;

    if (board_[nx][ny] == POINT_EMPTY) {
      is_eye = false;
      will_black_suicide = false;
      will_white_suicide = false;
    } else if (is_eye) {
      if (eye_type == POINT_EMPTY)
        eye_type = board_[nx][ny];
      else if (eye_type != board_[nx][ny])
        is_eye = false;
    }

    if (board_[nx][ny] == POINT_WHITE) {
      exist_white = true;
      if (liberties_[nx][ny] <= 1) {
        state_[x][y] = STATE(state_[x][y] | STATE_FORBID_BLACK);
      } else {
        will_white_suicide = false;
      }
    }
    if (board_[nx][ny] == POINT_BLACK) {
      exist_black = true;
      if (liberties_[nx][ny] <= 1) {
        state_[x][y] = STATE(state_[x][y] | STATE_FORBID_WHITE);
      } else {
        will_black_suicide = false;
      }
    }
  }
  if ((is_eye && eye_type == POINT_BLACK) ||
      (will_white_suicide && exist_white)) {
    state_[x][y] = STATE(state_[x][y] | STATE_FORBID_WHITE);
  }
  if ((is_eye && eye_type == POINT_WHITE) ||
      (will_black_suicide && exist_black)) {
    state_[x][y] = STATE(state_[x][y] | STATE_FORBID_BLACK);
  }
}

Node::Node(SmartBoard& board, const Coord& pos, Node* parent,
           POINT current_stone)
    : pos(pos), stone_type(current_stone), parent(parent) {
  if (pos.x != -1) board.Place(pos, current_stone);
  can_place = board.GetValidPlace(other_stone_type(current_stone));

  if (can_place.empty()) {
    terminal = true;
  }
  std::shuffle(can_place.begin(), can_place.end(), random_engine);
}
Node::~Node() {
  for (auto c : child) {
    delete c;
  }
}

Node* Node::expand(SmartBoard& board) {
  if (full_expanded == true) return nullptr;
  child.emplace_back(new Node(board, can_place[child.size()], this,
                              other_stone_type(stone_type)));
  if (child.size() == can_place.size()) {
    full_expanded = true;
  }
  return child[child.size() - 1];
}

Coord MCTsearch(SmartBoard& board, Node* root, POINT current_stone) {
  auto begin = std::chrono::steady_clock::now();
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                       begin) < TIME_LIMIT) {
    SmartBoard current = board;
    Node* leaf = tree_policy(current, root);
    double value = rollout(current, other_stone_type(leaf->stone_type));
    backup(leaf, value, other_stone_type(leaf->stone_type));

    LOOPTIMES++;
  }
  return best_child(root)->pos;
}

Node* tree_policy(SmartBoard& board, Node* root) {
  Node* result = root;
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

double rollout(SmartBoard& board, POINT current_stone) {
  POINT init_stone = current_stone;
  Coord p;
  while (p = rollout_policy(board, current_stone), p.x != -1) {
    board.Place(p, current_stone);
    current_stone = other_stone_type(current_stone);
  }
  if (init_stone == current_stone) {
    return 0.0;
  } else {
    return 1.0;
  }
}

Coord rollout_policy(const SmartBoard& board, POINT current_stone) {
  vector<Coord> can_place = board.GetValidPlace(current_stone);
  if (can_place.empty()) {
    return Coord{-1, -1};
  } else {
    return can_place[rand() % can_place.size()];
  }
}

void backup(Node* node, double result, POINT current_stone) {
  while (node != nullptr) {
    node->v += node->stone_type == current_stone ? result : 1 - result;
    node = node->parent;
  }
}

Node* best_child(Node* node) {
  auto result = *node->child.begin();
  for (auto i = node->child.begin(); i < node->child.end(); i++) {
    auto current = *i;
    current->ucb1 = (current->v) / (current->n + 1) +
                    C * std::sqrt(std::log(node->n + 1)) / (current->n + 1);
    if (current->ucb1 > result->ucb1) {
      result = current;
    } else if (current->ucb1 == result->ucb1 && rand() % 2 == 0) {
      result = current;
    }
  }
  return result;
}