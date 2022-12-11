#include <iostream>

#include "board.h"
#include "mcts.h"

using namespace cita;
using std::cin;
using std::cout;
using std::endl;
using std::string;

#ifdef _BOTZONE_ONLINE
#include <nlohmann/json.hpp>
#include <string>
using nlohmann::json;

// 在线环境使用json交互
int main() {
  POINT current = POINT_BLACK;
  Board board{};
  json input = json::parse(cin);
  int cnt{static_cast<int>(input["requests"].size() * 2 - 1)};
  for (int i = 0; i < cnt; i++) {
    string target{i % 2 == 0 ? "requests" : "responses"};
    int x{input[target][i / 2]["x"]}, y{input[target][i / 2]["y"]};

    if (x == -1) continue;

    board.Place(getPos(x, y), current);
    current = getOpp(current);
  }

  Node root(board, 0, nullptr, getOpp(current));
  DebugInfo debug{};
  Position result{UctSearch(board, &root, debug)};

  json response = {{"response", {{"x", getX(result)}, {"y", getY(result)}}},
                   {"debug", debug.info()}};

  cout << response.dump() << endl;
}
#else
// 本地调试环境使用简单交互
int main() {
  POINT current = POINT_BLACK;
  Board board{};
  int n{};
  cin >> n;
  int cnt{2 * n - 1};

  for (int i = 0; i < cnt; i++) {
    int x{}, y{};
    cin >> x >> y;
    if (x == -1) continue;

    board.Place(getPos(x, y), current);
    current = getOpp(current);
  }

  Node root(board, 0, nullptr, getOpp(current));
  DebugInfo debug{};
  Position result{UctSearch(board, &root, debug)};

  cout << getX(result) << ' ' << getY(result) << endl;
  cout << debug.info() << endl;
}
#endif