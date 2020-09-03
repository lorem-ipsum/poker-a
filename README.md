# Qt斗地主项目设计文档

## 大致框架

本程序在Ubuntu 20.04系统上编写，未使用Qt Designer，所有ui设计均通过代码实现。

三位玩家的游戏窗口分别由类`PlayerA``PlayerB``PlayerC`实现。其中`PlayerA`作为服务器端，另外两者作为客户端，分别接到服务器端上。

## 细节介绍

### `Cards`类

`cards.h`导出一个函数`bool CARDS_CMP(const QList<int>& c1, const QList<int>& c2)`用于判断两个整数数组对应的牌组的牌力大小。倘若`c1`能大过`c2`，则返回1.

```cpp
enum Suit;
enum CardsType;
struct Card{
  int value;
  Suit suit;
}
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


```

```cpp
class Background : public QWidget {
  Q_OBJECT
 public:
  Background(QWidget* parent = nullptr);
  ~Background();

 protected:
  void paintEvent(QPaintEvent* event); // 绘制静态的地图

 private:
};
```

### `CardLabel`类

`CardLabel`类用于在界面上显示扑克牌，且当玩家点击牌时，牌会向上移动；再次点击则移回原位。

```cpp
class CardLabel : public QLabel {
  Q_OBJECT
 public:
  CardLabel(QWidget* parent = nullptr);
  bool isMoved() { return moved_; }

 protected:
  void mousePressEvent(QMouseEvent* event) override;

 private:
  bool moved_ = false;
};
```

## 客户端工作流程

1. 本程序规定了各个客户端的连接顺序，因此必须以A->B->C的顺序点击"Connecting..."，建立连接。
2. 建立连接之后每个人会看到发给自己的牌。随机某方的界面上会显示"叫地主"和"不叫"两个按钮，点击之后将询问下一位是否叫地主。
3. 三个人都做过决策之后选出地主，把公共牌亮出并发给地主。从地主开始按上述顺序分别询问是否出牌，选定牌之后可以点击出牌，若出牌无法大过上家则会提示“不合法”。假如之前无人出过牌，或出牌之后另外两家均不出牌，则获得出牌权，此时可以出任意合法牌型，且不可不出。
4. 等到某一方出牌结束，三人的游戏界面会分别提示"You Win"或"You Lose"，游戏结束。此后三人的界面上会出现两个按钮"重新开始"和"退出"。点击退出则关闭页面；三人都点击重新开始之后(顺序无关)，重新开始一局游戏，从2)开始继续循环。

## 通信协议

本程序使用JSON数据在各个窗口之间通信。

- B和C向A发送B/C是否叫地主的信息：{"doYouWantToCampaign", "true/false"}
- B和C向A发送出牌信息：{"B/CHasPushedCards", "52.53."(王炸，举个例子，下同)}
- B和C向A发送不出的信息：{"B/CHasGivenUp", "info"(因为不需要第二个参数，故仅起占位作用，下同)}
- B和C向A发送重新开始的信息：{"restart", "info"}

- A向B和C发牌：{"cardsAssignedToB/C", "3.7.13.14.22.25.34.35..."}
- A询问B和C是否叫地主：{"doYouWantToCampaign", "info"}
- A告知B和C谁是地主：{"theLandlordIs", "2"}
- A告知B和C三张公共牌：{"commonCards", "14.48.51."}
- A询问B和C是否出牌：{"chuOrBuchu", "info"}
- A告知B和C有人出牌了：{"someOneHasPushedCards", "52.53.1."(最后一位表示谁出牌，012分别代表ABC，下同)}
- A告知B和C有人不出：{"someOneHasJustGivenUp", "2"}

## 规则设计流程

先实现一个工具类`Card`来存储牌型及大小(因为A和2之间不能连成顺，我们可以若将A的牌值设为14，则可将B的牌值设为16，小大王分别设为18和20)，然后实现一个主类`Cards`存储多个`Card`，维护总的牌型和`pivot`，pivot是该牌型的标志牌的大小(比如QQQA的`pivot`为Q，3334448K的`pivot`为3)。对于任意数量的`Card`，先按牌值从小到大进行排序。然后检查总的牌型。这通过分类讨论来完成。我们枚举牌的数量，当数量小于五时是平凡的。数量达到五以上时，先检查是否为顺子及连对，然后逐项检查是否为飞机。得到牌型之后就可以进行牌力比较了。

因为对于两个`Cards``c1`和`c2`，我们只需要知道`c1`是否能盖过`c2`，因此比较函数可以返回一个布尔变量。先检查二者的牌型是否为`CardType::ERROR`，若是，则返回`false`。然后判断二者之中是否存在王炸或炸弹(这也是平凡的)。倘若都没有，则检查二者张数和牌型是否相等，再比较二者的`pivot`。至此，牌力比较的部分已经完成。
