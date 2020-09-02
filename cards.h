#include <QList>
#include <algorithm>
#include <iostream>
#include <vector>

enum Suit { SPADE, HEART, DIAMOND, CLUB, JOKER };

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

int CardsCmp(const Cards& c1, const Cards& c2);

bool CARDS_CMP(const QList<int>& c1, const QList<int>& c2);

int intToCardValue(int x);

Suit intToCardType(int x);