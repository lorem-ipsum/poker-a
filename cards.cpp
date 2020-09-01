#include <algorithm>
#include <iostream>
#include <vector>

enum Suit { JOKER, SPADE, HEART, DIAMOND, CLUB };

enum CardsType {
  HIGH,        // 0 高牌
  PAIR,        // 1 一对
  THREE,       // 2 三条
  THREE_ONE,   // 3 三带一
  THREE_TWO,   // 4 三带二
  FOUR,        // 5 炸弹
  FOUR_TWO,    // 6 四带二
  FOUR_PAIRS,  // 7 四带两对
  PLANE,       // 8 飞机
  ROW,         // 9 顺子
  ROW2,        // 10 双顺
  ROCKET,      // 11 王炸
  ERROR,       // 12 没法儿出
};

struct Card {
  Card(int v, Suit s) : value(v), suit(s) {}
  bool operator==(const Card& rhs) { return value == rhs.value; }
  bool operator!=(const Card& rhs) { return value != rhs.value; }
  int value;  // 3-13, 14~A, 16~2, 18~小王, 20~大王
  Suit suit;
};

class Cards {
 private:
  std::vector<Card> cards_;

  void sort_cards();
  CardsType check_type();

  CardsType type_;
  int pivot_;

 public:
  Cards(const std::vector<Card>& vec) : cards_(vec) {
    sort_cards();
    type_ = check_type();
  }
  CardsType type() { return type_; }

  // c1输或平局: -1, c1赢: 1, 无法比较: 0
  friend int CardsCmp(const Cards& c1, const Cards& c2);
};

void Cards::sort_cards() {
  std::sort(cards_.begin(), cards_.end(), [](const Card& c1, const Card& c2) {
    return c1.value != c2.value ? c1.value < c2.value : c1.suit < c2.suit;
  });
}

CardsType Cards::check_type() {
  if (cards_.size() == 1) {
    pivot_ = cards_[0].value;
    return HIGH;
  }
  if (cards_.size() == 2) {
    if (cards_[0].suit == JOKER && cards_[1].suit == JOKER) {
      pivot_ = 99;
      return ROCKET;
    }
    if (cards_[0] == cards_[1]) {
      pivot_ = cards_[0].value;
      return PAIR;
    }
    return CardsType::ERROR;
  }
  if (cards_.size() == 3) {
    CardsType type = cards_[0] == cards_[1] && cards_[1] == cards_[2]
                         ? THREE
                         : CardsType::ERROR;
    if (type == THREE) {
      pivot_ = cards_[0].value;
      return type;
    } else {
      pivot_ = -1;
      return type;
    }
  }

  if (cards_.size() == 4) {
    pivot_ = cards_[2].value;
    if (cards_[0] == cards_[3]) return FOUR;
    if (cards_[0] == cards_[2] && cards_[2] != cards_[3]) return THREE_ONE;
    if (cards_[3] == cards_[1] && cards_[1] != cards_[0]) return THREE_ONE;
    pivot_ = -1;
    return CardsType::ERROR;
  }

  // 判断是否成顺
  // 单顺
  bool row = true;
  for (int i = 0; i < cards_.size() - 1; ++i) {
    if (cards_[i].value + 1 != cards_[i + 1].value) {
      row = false;
    }
  }
  if (row) {
    pivot_ = cards_[0].value;
    return ROW;
  }

  // 双顺
  if (cards_.size() % 2 == 0) {
    bool row2 = true;
    for (int i = 0; i < cards_.size() - 3; i += 2) {
      if (!(cards_[i] == cards_[i + 1] &&
            cards_[i + 1].value + 1 == cards_[i + 2].value)) {
        row2 = false;
      }
    }
    if (cards_[cards_.size() - 1] != cards_[cards_.size() - 2]) row2 = false;
    if (row2) {
      pivot_ = cards_[0].value;
      return ROW2;
    }
  }

  // 三带二
  if (cards_.size() == 5) {
    pivot_ = cards_[2].value;
    if (cards_[0] == cards_[1] && cards_[1] == cards_[2] &&
        cards_[3] == cards_[4])
      return THREE_TWO;
    if (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
        cards_[3] == cards_[4])
      return THREE_TWO;
    pivot_ = -1;
    return CardsType::ERROR;
  }

  // 飞机

  // n*3+0 注意与3*3+1冲突
  if (cards_.size() % 3 == 0) {
    bool n3 = true;
    for (int i = 0; i < cards_.size() / 3; ++i) {
      if (cards_[3 * i] != cards_[3 * i + 2]) n3 = false;
      if (3 * i + 3 < cards_.size() &&
          cards_[3 * i + 2].value + 1 != cards_[3 * i + 3].value)
        n3 = false;
    }
    if (n3) {
      pivot_ = cards_[0].value;
      return PLANE;
    }
  }
  // 2*3+1 注意与四带两对冲突
  if (cards_.size() == 8) {
    for (int i = 0; i < 3; ++i) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value) {
        pivot_ = cards_[i].value;
        return PLANE;
      }
    }
  }
  // 2*3+2
  if (cards_.size() == 10) {
    CardsType type;
    for (int i = 0; i <= 4; i += 2) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value) {
        if (i == 0) {
          type = (cards_[6] == cards_[7] && cards_[8] == cards_[9])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 2) {
          type = (cards_[0] == cards_[1] && cards_[8] == cards_[9])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 4) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3])
                     ? PLANE
                     : CardsType::ERROR;
        }
        if (type == PLANE) {
          pivot_ = cards_[i].value;
          return PLANE;
        } else {
          pivot_ = -1;
          return CardsType::ERROR;
        }
      }
      return CardsType::ERROR;
    }
  }
  // 3*3+1 注意与4*3冲突
  if (cards_.size() == 12) {
    for (int i = 0; i <= 3; ++i) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 6] == cards_[i + 8] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value &&
          cards_[i + 5].value + 1 == cards_[i + 6].value) {
        pivot_ = cards_[i].value;
        return PLANE;
      }
    }
  }
  // 3*3+2
  if (cards_.size() == 15) {
    CardsType type;
    for (int i = 0; i <= 6; i += 2) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 6] == cards_[i + 8] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value &&
          cards_[i + 5].value + 1 == cards_[i + 6].value) {
        if (i == 0) {
          type = (cards_[9] == cards_[10] && cards_[11] == cards_[12] &&
                  cards_[13] == cards_[14])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 2) {
          type = (cards_[0] == cards_[1] && cards_[11] == cards_[12] &&
                  cards_[13] == cards_[14])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 4) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
                  cards_[13] == cards_[14])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 6) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
                  cards_[4] == cards_[5])
                     ? PLANE
                     : CardsType::ERROR;
        }
        if (type == PLANE) {
          pivot_ = cards_[i].value;
          return PLANE;
        } else {
          pivot_ = -1;
          return CardsType::ERROR;
        }
      }
      return CardsType::ERROR;
    }
  }
  // 4*3+1
  if (cards_.size() == 16) {
    for (int i = 0; i < 4; ++i) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 6] == cards_[i + 8] && cards_[i + 9] == cards_[i + 11] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value &&
          cards_[i + 5].value + 1 == cards_[i + 6].value &&
          cards_[i + 8].value + 1 == cards_[i + 9].value) {
        pivot_ = cards_[i].value;
        return PLANE;
      }
    }
    return CardsType::ERROR;
  }
  // 4*3+2
  if (cards_.size() == 20) {
    CardsType type;
    for (int i = 0; i <= 8; i += 2) {
      if (cards_[i + 0] == cards_[i + 2] && cards_[i + 3] == cards_[i + 5] &&
          cards_[i + 6] == cards_[i + 8] && cards_[i + 9] == cards_[i + 11] &&
          cards_[i + 2].value + 1 == cards_[i + 3].value &&
          cards_[i + 5].value + 1 == cards_[i + 6].value &&
          cards_[i + 8].value + 1 == cards_[i + 9].value) {
        if (i == 0) {
          type = (cards_[12] == cards_[13] && cards_[14] == cards_[15] &&
                  cards_[16] == cards_[17] && cards_[18] == cards_[19])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 2) {
          type = (cards_[0] == cards_[1] && cards_[14] == cards_[15] &&
                  cards_[16] == cards_[17] && cards_[18] == cards_[19])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 4) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
                  cards_[16] == cards_[17] && cards_[18] == cards_[19])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 6) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
                  cards_[4] == cards_[5] && cards_[18] == cards_[19])
                     ? PLANE
                     : CardsType::ERROR;
        } else if (i == 8) {
          type = (cards_[0] == cards_[1] && cards_[2] == cards_[3] &&
                  cards_[4] == cards_[5] && cards_[6] == cards_[7])
                     ? PLANE
                     : CardsType::ERROR;
        }
        if (type == PLANE) {
          pivot_ = cards_[i].value;
          return PLANE;
        } else {
          pivot_ = -1;
          return CardsType::ERROR;
        }
      }
      return CardsType::ERROR;
    }
  }
  // 四带二
  if (cards_.size() == 6) {
    for (int i = 0; i <= 2; ++i) {
      if (cards_[i] == cards_[i + 3]) {
        pivot_ = cards_[i].value;
        return FOUR_TWO;
      }
    }
    pivot_ = -1;
    return CardsType::ERROR;
  }

  // 四带两对
  if (cards_.size() == 8) {
    if (cards_[0] == cards_[3] && cards_[4] == cards_[5] &&
        cards_[6] == cards_[7]) {
      pivot_ = cards_[0].value;
      return FOUR_PAIRS;
    }
    if (cards_[2] == cards_[5] && cards_[0] == cards_[1] &&
        cards_[6] == cards_[7]) {
      pivot_ = cards_[2].value;
      return FOUR_PAIRS;
    }
    if (cards_[4] == cards_[7] && cards_[0] == cards_[1] &&
        cards_[2] == cards_[3]) {
      pivot_ = cards_[4].value;
      return FOUR_PAIRS;
    }
    return CardsType::ERROR;
  }

  std::cout << "Oooops\n";
  pivot_ = -1;
  return CardsType::ERROR;
}

int CardsCmp(const Cards& c1, const Cards& c2) {
  if (c1.type_ == CardsType::ERROR || c2.type_ == CardsType::ERROR) return 0;

  // 某人有王炸
  if (c1.type_ == ROCKET) return 1;
  if (c2.type_ == ROCKET) return -1;

  // 某人有炸弹
  if ((c1.type_ == FOUR) ^ (c2.type_ == FOUR)) return c1.type_ == FOUR ? 1 : -1;
  if (c1.type_ == FOUR && c2.type_ == FOUR)
    return c1.cards_[0].value > c2.cards_[0].value ? 1 : -1;

  if (c1.type_ != c2.type_) return 0;

  // 高牌，一对，两对，三条，三带一二，四带一二
  if (static_cast<int>(c1.type_) <= 7) {
    return c1.pivot_ > c2.pivot_ ? 1 : -1;
  }

  // 飞机、顺子、双顺
  if (c1.type_ == 8 || c1.type_ == 9 || c1.type_ == 10) {
    if (c1.cards_.size() != c2.cards_.size()) return 0;
    return c1.pivot_ > c2.pivot_ ? 1 : -1;
  }

  std::cout << "Oops\n";
  return 0;
}

int main() {
  std::vector<Card> vec, vec2;
  vec.push_back({3, SPADE});
  vec.push_back({6, SPADE});
  vec.push_back({10, SPADE});
  vec.push_back({10, SPADE});
  vec.push_back({10, SPADE});
  vec.push_back({9, SPADE});
  vec.push_back({9, SPADE});
  vec.push_back({9, SPADE});
  vec.push_back({8, SPADE});
  vec.push_back({8, SPADE});
  vec.push_back({8, SPADE});
  vec.push_back({7, SPADE});

  vec2.push_back({10, SPADE});
  vec2.push_back({10, SPADE});
  vec2.push_back({10, SPADE});
  vec2.push_back({10, SPADE});

  Cards c(vec);
  Cards c2(vec2);

  // std::cout << c.type() << std::endl;

  std::cout << CardsCmp(c2, c) << std::endl;
  return 0;
}