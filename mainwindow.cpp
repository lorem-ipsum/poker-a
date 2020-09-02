#include "mainwindow.h"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QTime>
#include <algorithm>
#include <ctime>

PlayerA::PlayerA(QWidget* parent) : QMainWindow(parent) {
  setFixedSize(1200, 800);

  // startListening button
  buttonStartListening = new QPushButton(this);
  buttonStartListening->setGeometry(376, 512, 448, 120);
  buttonStartListening->setFont(QFont("Helvetica", 28));
  buttonStartListening->setText("Start connecting...");
  connect(buttonStartListening, SIGNAL(clicked()), this,
          SLOT(startListening()));
  connect(this, SIGNAL(allHaveCalled()), this, SLOT(afterAllHaveCalled()));

  // Game started
}

void PlayerA::startListening() {
  serverA_ = new QTcpServer();
  serverA_->listen(QHostAddress::Any, portA_);
  connect(serverA_, &QTcpServer::newConnection, this,
          &PlayerA::handleConnections);
}

PlayerA::~PlayerA() {}

void PlayerA::handleConnections() {
  if (!socketAB_) {
    qDebug() << "Server: new connection from B";
    socketAB_ = serverA_->nextPendingConnection();
  } else {
    qDebug() << "Server: new connection from C";
    socketAC_ = serverA_->nextPendingConnection();
  }

  if (socketAB_ && socketAC_) {
    connect(socketAB_, &QTcpSocket::readyRead, this,
            &PlayerA::socketReadDataFromB);
    connect(socketAB_, &QTcpSocket::disconnected, this,
            &PlayerA::socketDisconnectedFromB);

    connect(socketAC_, &QTcpSocket::readyRead, this,
            &PlayerA::socketReadDataFromC);
    connect(socketAC_, &QTcpSocket::disconnected, this,
            &PlayerA::socketDisconnectedFromC);

    // boardCast("thereAreThreePeople", "true");
    startCampaign();
  }
}

// 大致上是对方write时调用
void PlayerA::socketReadDataFromB() {
  qDebug() << "A IS READING FROM BUFFER FROM B";
  QByteArray buffer = socketAB_->readAll();

  QString doesBCampaign = readFromBuffer(buffer, "doYouWantToCampaign");
  if (!doesBCampaign.isEmpty()) {
    showOthersIfCampaignInfo(1, doesBCampaign == "true");
    ifWantToCampaign(1, doesBCampaign == "true");
    askIfCallLandlord(2);
  }

  QString BPushedCards = readFromBuffer(buffer, "BHasPushedCards");
  if (!BPushedCards.isEmpty()) {
    lastPushCardPerson_ = 1;
    QList<int> bcards = stringToIntArray(BPushedCards);
    cardsOnTable_ = bcards;
    showTableOnSelfScreen(cardsOnTable_);
    castToC("someOneHasPushedCards", BPushedCards);
    sleep(50);
    castToC("chuOrBuchu", "info");
  }

  QString BHasGivenUp = readFromBuffer(buffer, "BHasGivenUp");
  if (!BHasGivenUp.isEmpty()) {
    // //展示，并发给C
    // displayGiveUpInfo(1);
    // //让C出牌
    // castToC("chuOrBuchu", "info");
    giveUp(1);
  }
}
void PlayerA::socketReadDataFromC() {
  qDebug() << "A IS READING FROM BUFFER FROM C";
  QByteArray buffer = socketAC_->readAll();

  QString doesCCampaign = readFromBuffer(buffer, "doYouWantToCampaign");
  if (!doesCCampaign.isEmpty()) {
    showOthersIfCampaignInfo(2, doesCCampaign == "true");
    ifWantToCampaign(2, doesCCampaign == "true");
    askIfCallLandlord(0);
  }

  QString CPushedCards = readFromBuffer(buffer, "CHasPushedCards");
  if (!CPushedCards.isEmpty()) {
    lastPushCardPerson_ = 2;
    QList<int> ccards = stringToIntArray(CPushedCards);
    cardsOnTable_ = ccards;
    showTableOnSelfScreen(cardsOnTable_);
    castToB("someOneHasPushedCards", CPushedCards);
    sleep(50);
    showChuOrBuchuBtns();
  }

  QString CHasGivenUp = readFromBuffer(buffer, "CHasGivenUp");
  if (!CHasGivenUp.isEmpty()) {
    //展示，并发给B
    giveUp(2);
  }
}
void PlayerA::socketDisconnectedFromB() {}
void PlayerA::socketDisconnectedFromC() {}

void PlayerA::startCampaign() {
  QTime time = QTime::currentTime();
  qsrand(time.msec() + time.second() * 1000);
  int n = qrand() % 3;

  buttonStartListening->hide();

  // n is the first to call
  firstCampaign(n);
}

void PlayerA::firstCampaign(int n) {
  // n is the first to call
  // boardCast("firstCall", QString::number(n));
  QVector<int> cards_index;
  for (int i = 0; i < 54; ++i) {
    cards_index.push_back(i);
  }
  srand(time(nullptr));
  std::random_shuffle(cards_index.begin(), cards_index.end());
  std::sort(cards_index.begin(), cards_index.begin() + 17);
  std::sort(cards_index.begin() + 17, cards_index.begin() + 34);
  std::sort(cards_index.begin() + 34, cards_index.begin() + 51);
  std::sort(cards_index.begin() + 51, cards_index.begin() + 54);

  for (int i = 0; i < 17; ++i) {
    cardsOfA_.append(cards_index[i]);
  }
  displayCards();

  QString cb;
  for (int i = 17; i < 34; ++i) {
    cb.push_back(QString::number(cards_index[i]));
    cb.push_back('.');
  }
  castToB("cardsAssignedToB", cb);

  QString cc;
  for (int i = 34; i < 51; ++i) {
    cc.push_back(QString::number(cards_index[i]));
    cc.push_back('.');
  }
  castToC("cardsAssignedToC", cc);

  for (int i = 51; i < 54; ++i) {
    commonCards_.append(cards_index[i]);
  }

  n = 1;
  currentLandlord_ = n;
  askIfCallLandlord(n);
}

void PlayerA::askIfCallLandlord(int n) {
  qDebug() << "ASKIFCALLLANDLORD" << n;
  // currentLandlord_ = n;

  // 检查是否所有人都叫过地主了
  ++currentCallNumber_;
  if (currentCallNumber_ == 4) {
    emit allHaveCalled();
    return;
  }
  // currentLandlord_ = n;

  if (n == 0) {
    //自己弹窗
    // QButtonGroup* doYouWantToCampaign = new QButtonGroup(this);
    QPushButton* yesBtn = new QPushButton(this);
    QPushButton* noBtn = new QPushButton(this);

    yesBtn->setText("Yes");
    noBtn->setText("No");
    yesBtn->setGeometry(120, 220, 50, 50);
    noBtn->setGeometry(180, 220, 50, 50);
    yesBtn->show();
    noBtn->show();
    // doYouWantToCampaign->addButton(yesBtn);
    // doYouWantToCampaign->addButton(noBtn);

    connect(yesBtn, SIGNAL(clicked()), yesBtn, SLOT(hide()));
    connect(yesBtn, SIGNAL(clicked()), noBtn, SLOT(hide()));
    connect(noBtn, SIGNAL(clicked()), yesBtn, SLOT(hide()));
    connect(noBtn, SIGNAL(clicked()), noBtn, SLOT(hide()));

    connect(yesBtn, &QPushButton::clicked,
            [=]() { ifWantToCampaign(0, true); });
    connect(noBtn, &QPushButton::clicked,
            [=]() { ifWantToCampaign(0, false); });

    connect(yesBtn, &QPushButton::clicked, [=]() {
      currentLandlord_ = 0;
      askIfCallLandlord(1);
    });
    connect(noBtn, &QPushButton::clicked, [=]() { askIfCallLandlord(1); });
  } else if (n == 1) {
    //问B
    sleep(50);
    castToB("doYouWantToCampaign", "info");
  } else if (n == 2) {
    sleep(50);
    castToC("doYouWantToCampaign", "info");
  }
}

void PlayerA::ifWantToCampaign(int n, bool b) {
  qDebug() << "A send ifWantToCampaign info";
  if (b) {
    currentLandlord_ = n;
    boardCast("ifWantToCampaign", QString::number(10 * n + 1));
  } else {
    boardCast("ifWantToCampaign", QString::number(10 * n + 0));
  }

  showOthersIfCampaignInfo(n, b);
}

void PlayerA::showOthersIfCampaignInfo(int personPosition, bool ifCampaign) {
  if (personPosition == 0) {
    QLabel* jiaoORbujiao = new QLabel(this);
    jiaoORbujiao->setText(ifCampaign ? "叫" : "不叫");
    jiaoORbujiao->setGeometry(580, 400, 40, 32);
    jiaoORbujiao->show();
  } else if (personPosition == 1) {
    QLabel* jiaoORbujiao = new QLabel(this);
    jiaoORbujiao->setText(ifCampaign ? "叫" : "不叫");
    jiaoORbujiao->setGeometry(160, 200, 40, 32);
    jiaoORbujiao->show();
  } else if (personPosition == 2) {
    QLabel* jiaoORbujiao = new QLabel(this);
    jiaoORbujiao->setText(ifCampaign ? "叫" : "不叫");
    jiaoORbujiao->setGeometry(960, 200, 40, 32);
    jiaoORbujiao->show();
  }
}

void PlayerA::displayCards() {
  // 先删除之前的卡
  for (QLabel* item : cardLabels_) {
    item->hide();
  }
  cardLabels_.clear();

  // SHOW
  for (int i = 0; i < cardsOfA_.size(); ++i) {
    CardLabel* cardLabel = new CardLabel(this);
    cardLabels_.append(cardLabel);
    QPixmap cardImage =
        QPixmap("./assets/" + QString::number(cardsOfA_[i]) + ".png");
    cardImage = cardImage.scaled(QSize(330, 160), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    cardLabel->setPixmap(cardImage);
    cardLabel->setGeometry(80 + 50 * i, 600, 360, 200);
    cardLabel->show();
  }
}

void PlayerA::displayCommonCards(QList<int> commonCards) {
  for (int i = 0; i < commonCards.size(); ++i) {
    CardLabel* cardLabel = new CardLabel(this);
    QPixmap cardImage =
        QPixmap("./assets/" + QString::number(commonCards[i]) + ".png");
    cardImage = cardImage.scaled(QSize(330, 160), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    cardLabel->setPixmap(cardImage);
    cardLabel->setGeometry(500 + 50 * i, 50, 360, 200);
    cardLabel->show();
  }
}

void PlayerA::afterAllHaveCalled() {
  qDebug() << "After all have called";
  sleep(50);
  boardCast("commonCards", intArrayToString(commonCards_));
  displayCommonCards(commonCards_);
  if (currentLandlord_ == 0) {
    cardsOfA_.append(commonCards_);
    std::sort(cardsOfA_.begin(), cardsOfA_.end());
    displayCards();
  }
  sleep(50);
  boardCast("theLandlordIs", QString::number(currentLandlord_));

  startPlaying();
}

void PlayerA::startPlaying() {
  qDebug() << "START Playing!";
  if (currentLandlord_ == 0) {
    showChuOrBuchuBtns();
  } else if (currentLandlord_ == 1) {
    castToB("chuOrBuchu", "info");
  } else if (currentLandlord_ == 2) {
    castToC("chuOrBuchu", "info");
  }
}

void PlayerA::showChuOrBuchuBtns() {
  qDebug() << "A chu or buchu";
  QPushButton* chuBtn = new QPushButton(this);
  QPushButton* buchuBtn = new QPushButton(this);

  chuBtn->setText("出牌");
  buchuBtn->setText("不出");
  chuBtn->setGeometry(120, 220, 50, 50);
  buchuBtn->setGeometry(180, 220, 50, 50);
  chuBtn->show();
  buchuBtn->show();

  // connect(chuBtn, SIGNAL(clicked()), chuBtn, SLOT(hide()));
  // connect(chuBtn, SIGNAL(clicked()), buchuBtn, SLOT(hide()));
  connect(buchuBtn, SIGNAL(clicked()), chuBtn, SLOT(hide()));
  connect(buchuBtn, SIGNAL(clicked()), buchuBtn, SLOT(hide()));

  buchuBtn->setDisabled(lastPushCardPerson_ == 0);
  if (lastPushCardPerson_ == 0) {
    cardsOnTable_ = {};
  }

  // btn connect
  connect(chuBtn, &QPushButton::clicked, [=]() {
    QList<int> cardsToPush;
    for (int i = 0; i < cardsOfA_.size(); ++i) {
      if (cardLabels_[i]->isMoved()) {
        cardsToPush.append(cardsOfA_[i]);
      }
    }

    qDebug() << "CHECKING...1";
    if (CARDS_CMP(cardsToPush, cardsOnTable_)) {
      qDebug() << "CHECKING...2";

      // A不再展示出掉的牌
      for (int i = 0; i < cardsToPush.size(); ++i) {
        cardsOfA_.removeOne(cardsToPush[i]);
      }
      displayCards();

      someOneHasJustPushedCards(cardsToPush);
      lastPushCardPerson_ = 0;
      // cardsOnTable_ = cardsToPush;
      chuBtn->hide();
      buchuBtn->hide();
    }
    sleep(50);
    castToB("chuOrBuchu", "info");
  });
  connect(buchuBtn, &QPushButton::clicked, [=]() { giveUp(0); });
}

// void PlayerA::pushCards() {
//   QList<int> cardsToPush;
//   for (int i = 0; i < cardsOfA_.size(); ++i) {
//     if (cardLabels_[i]->isMoved()) {
//       cardsToPush.append(cardsOfA_[i]);
//     }
//   }

//   qDebug() << "CHECKING...1";
//   if (CARDS_CMP(cardsToPush, cardsOnTable_)) {
//     qDebug() << "CHECKING...2";

//     someOneHasJustPushedCards(cardsToPush);
//     lastPushCardPerson_ == 0;
//     // cardsOnTable_ = cardsToPush;
//   }
//   sleep(50);
//   castToB("chuOrBuchu", "info");
// }

void PlayerA::someOneHasJustPushedCards(QList<int> cardsToPush) {
  cardsOnTable_ = cardsToPush;
  showTableOnSelfScreen(cardsOnTable_);
  sleep(50);
  boardCast("someOneHasPushedCards", intArrayToString(cardsToPush));
}

void PlayerA::showTableOnSelfScreen(QList<int>) {
  // 先删除之前的卡
  for (QLabel* item : tableCardLabels_) {
    item->hide();
  }
  tableCardLabels_.clear();

  // SHOW
  for (int i = 0; i < cardsOnTable_.size(); ++i) {
    CardLabel* cardLabel = new CardLabel(this);
    tableCardLabels_.append(cardLabel);
    QPixmap cardImage =
        QPixmap("./assets/" + QString::number(cardsOnTable_[i]) + ".png");
    cardImage = cardImage.scaled(QSize(330, 160), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    cardLabel->setPixmap(cardImage);
    cardLabel->setGeometry(500 + 50 * i, 250, 360, 200);
    cardLabel->show();
  }
}

// 第n人放弃了，转交给下一个人
void PlayerA::giveUp(int n) {
  displayGiveUpInfo(n);
  boardCast("someOneHasJustGivenUp", QString::number(n));
  sleep(50);
  if (n == 0)
    castToB("chuOrBuchu", "info");
  else if (n == 1)
    castToC("chuOrBuchu", "info");
  else if (n == 2)
    showChuOrBuchuBtns();
}

void PlayerA::displayGiveUpInfo(int n) {
  int personPosition = personIndexToPosition_[n];

  if (personPosition == 0) {
    QLabel* chuORbuchu = new QLabel(this);
    chuORbuchu->setText("不出");
    chuORbuchu->setGeometry(580, 400, 40, 32);
    chuORbuchu->show();
  } else if (personPosition == 1) {
    QLabel* chuORbuchu = new QLabel(this);
    chuORbuchu->setText("不出");
    chuORbuchu->setGeometry(160, 200, 40, 32);
    chuORbuchu->show();
  } else if (personPosition == 2) {
    QLabel* chuORbuchu = new QLabel(this);
    chuORbuchu->setText("不出");
    chuORbuchu->setGeometry(960, 200, 40, 32);
    chuORbuchu->show();
  }
}

void PlayerA::askTheNextIfWantToChu(int n) {}
