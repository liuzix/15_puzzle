#include <iostream>
#include <assert.h>
#include <utility>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <string>

using namespace std;

#define BOARD_SIZE 16
#define BOARD_DIM 4

enum Direction {up, down, left, right};
struct GameBoard {
    uint8_t *board;
    uint8_t fscore, gscore;
    GameBoard* previous;

    GameBoard() {
      fscore = 0;
      gscore = 0;
      board = new uint8_t[BOARD_SIZE];
      previous = nullptr;
    }

    GameBoard apply_move(Direction move) const {
      GameBoard ret;
      ret.fscore = this->fscore + (uint8_t) 1;
      std::memcpy(ret.board, this->board, BOARD_SIZE);
      ret.previous = const_cast<GameBoard*>(this);

      std::pair<int, int> hole_pos = get_hole_pos();
      int x = hole_pos.first;
      int y = hole_pos.second;
      int x_origin = x;
      int y_origin = y;
      switch (move) {
        case Direction::up:
          y -= 1;
          break;
        case Direction::down:
          y += 1;
          break;
        case Direction::left:
          x -= 1;
          break;
        case Direction::right:
          x += 1;
          break;
      }

      ret.pos(x_origin, y_origin) = ret.pos(x, y);
      ret.pos(x, y) = BOARD_SIZE;

      ret.eval_gscore();
      return ret;
    }

    uint8_t& pos(int x, int y) const {
      assert(x < BOARD_DIM && y < BOARD_DIM);
      int offset = x + BOARD_DIM * y;
      assert(offset < BOARD_SIZE);
      return board[offset];
    }

    bool is_goal () const {
      for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] != i + 1) return false;
      }
      return true;
    };

    uint8_t eval_gscore() {
      uint8_t g = 0;
      for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] != i + 1) g++;
      }
      gscore = g;
      return g;
    }

    vector<Direction> get_possible_moves () const {
      vector<Direction> ret;
      std::pair<int, int> hole_pos;
      hole_pos = get_hole_pos();
      if (hole_pos.first != 0) ret.push_back(Direction::left);
      if (hole_pos.first != BOARD_DIM - 1) ret.push_back(Direction::right);
      if (hole_pos.second != 0) ret.push_back(Direction::up);
      if (hole_pos.second != BOARD_DIM - 1) ret.push_back(Direction::down);

      return ret;
    }

    static bool compare_gscore(GameBoard const &a, GameBoard const &b) {
      assert(a.gscore <= 16  && b.gscore <= 16);
      return (a.fscore + a.gscore > b.fscore + b.gscore);
    }

    static bool compare_content(GameBoard const &a, GameBoard const &b) {
      for (int i = 0; i < BOARD_SIZE; i++) {
        if (a.board[i] < b.board[i]) return true;
      }
      return false;
    }

    bool operator==(GameBoard const& game) const {
      if (this->gscore != game.gscore) return false;
      for (int i = 0; i < BOARD_SIZE; i++) {
        if (this->board[i] != game.board[i]) return false;
      }
      return true;
    }
private:
    std::pair<int, int> off_to_pos(uint8_t off) const {
      int y = off / BOARD_DIM;
      int x = off % BOARD_DIM;
      return std::make_tuple(x, y);
    }

    std::pair<int, int> get_hole_pos() const {
      std::pair<int, int> hole_pos;
      int i;
      for (i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == BOARD_SIZE) break;
      }
      assert(i < BOARD_SIZE);
      hole_pos = off_to_pos(i);
      return hole_pos;
    }
};

struct MyVector {
    vector<GameBoard> data_container;

    MyVector() {
      //data_container.reserve(1024);
      make_heap(data_container.begin(), data_container.end(), GameBoard::compare_gscore);
    }

    void insert(GameBoard const& g) {
      data_container.push_back(g);
      push_heap(data_container.begin(), data_container.end(), GameBoard::compare_gscore);
    }

    GameBoard pop_front() {
      //pop_heap(data_container.begin(), data_container.end(), GameBoard::compare_gscore);
      sort_heap (data_container.begin(), data_container.end(), GameBoard::compare_gscore);
      GameBoard ret = data_container.back();
      data_container.pop_back();
      make_heap(data_container.begin(), data_container.end(), GameBoard::compare_gscore);
      return ret;
    }

    int has_member(GameBoard const& g) {
      vector<GameBoard>::iterator iter;
      iter = find(data_container.begin(), data_container.end(), g);
      if (iter == data_container.end()) return -1;
      return iter->fscore;
    }

    bool is_empty() {
      return data_container.empty();
    }


};

MyVector openset;
set<GameBoard, decltype(&GameBoard::compare_content)> closedset(GameBoard::compare_content);

GameBoard read_from_stdin();
void evaluate_one_node(GameBoard const& game);
GameBoard solve(GameBoard const& start);

int main() {
  GameBoard start;
  start = read_from_stdin();
  closedset.insert(start);
  GameBoard test(start);
  uint8_t * test_buf = new uint8_t[BOARD_SIZE];
  std::memcpy(test_buf, start.board, BOARD_SIZE);
  test.board = test_buf;
  test.gscore = 99;
  assert(closedset.find(test) != closedset.end());
  GameBoard solution = solve(start);
  std::cout << "found a solution!" << std::endl;
  return 0;
}

GameBoard solve(GameBoard const& start) {
  openset.insert(start);
  while (!openset.is_empty()) {
    GameBoard this_node = openset.pop_front();
    if (this_node.is_goal()) {
      return this_node;
    }
    std::cout << (int)this_node.gscore << std::endl;
    evaluate_one_node(this_node);

  }
  assert(!"No solution found!");
}

void evaluate_one_node(GameBoard const& game) {
  vector<Direction> moves = game.get_possible_moves();
  assert(moves.size() >= 2);
  for (Direction move: moves) {
    GameBoard new_node(game.apply_move(move));
    if (closedset.find(new_node) != closedset.end()) { // already traversed
      delete[] new_node.board;
      continue;
    } else {
      closedset.insert(new_node);
    }

    int res = openset.has_member(new_node);
    if (-1 == res) { // this is a new node
      openset.insert(new_node);
    } else if (res < new_node.fscore) { // if the new node is worse than the old
      delete[] new_node.board; // free memory
      continue;
    }

  }
}

GameBoard read_from_stdin() {
  string buf;
  int temp;
  GameBoard ret;
  for (int i = 0; i < BOARD_SIZE; i++) {
    std::cin >> buf;
    if (buf == "-") {
      temp = BOARD_SIZE;
    } else {
      temp = stoi(buf);
    }
    assert(temp >= 0 && temp <= BOARD_SIZE);
    ret.board[i] = temp;
  }
  ret.eval_gscore();
  return ret;
}