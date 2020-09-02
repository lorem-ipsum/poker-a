#include "mainwindow.h"

#include <QApplication>
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

    updateCardNumber(1, bcards.size());
    cardsOnTable_ = bcards;
    showTableOnSelfScreen(cardsOnTable_);
    // BPushedCards.append(1);
    bcards.append(1);
    castToC("someOneHasPushedCards", intArrayToString(bcards));
    sleep(50);

    if (checkIfGameOver()) return;

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

  // QString doesLandlordWin = readFromBuffer(buffer, "gameOver");
  // if (!doesLandlordWin.isEmpty()) {
  //   qDebug() << "B has won!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  //   landlordWins_ = doesLandlordWin == "true";
  //   castToC("gameOver",
  //           (landlordWins_ ^ (currentLandlord_ != 2)) ? "true" : "false");
  //   showWinOrLoseInfo();
  //   showRestartOrExitBtnsOnSelfScreen();
  // }
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

    updateCardNumber(2, ccards.size());
    cardsOnTable_ = ccards;
    showTableOnSelfScreen(cardsOnTable_);
    // BPushedCards.append(1);
    ccards.append(2);
    castToB("someOneHasPushedCards", intArrayToString(ccards));
    sleep(50);

    if (checkIfGameOver()) return;

    showChuOrBuchuBtns();
  }

  QString CHasGivenUp = readFromBuffer(buffer, "CHasGivenUp");
  if (!CHasGivenUp.isEmpty()) {
    //展示，并发给B
    giveUp(2);
  }

  // QString doesLandlordWin = readFromBuffer(buffer, "gameOver");
  // qDebug() << "doesLandlordWin = " << doesLandlordWin <<
  // "********************"; if (!doesLandlordWin.isEmpty()) {
  //   qDebug() << "C has won!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  //   landlordWins_ = doesLandlordWin == "true";
  //   castToB("gameOver",
  //           (landlordWins_ ^ (currentLandlord_ != 1)) ? "true" : "false");
  //   showWinOrLoseInfo();
  //   showRestartOrExitBtnsOnSelfScreen();
  // }
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
    yesBtn->setGeometry(120, 500, 50, 50);
    noBtn->setGeometry(180, 500, 50, 50);
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
  QLabel* jiaoORbujiao = new QLabel(this);
  jiaoORbujiaoLabels_.append(jiaoORbujiao);
  jiaoORbujiao->setText(ifCampaign ? "叫" : "不叫");
  if (personPosition == 0) {
    jiaoORbujiao->setGeometry(580, 400, 40, 32);
  } else if (personPosition == 1) {
    jiaoORbujiao->setGeometry(160, 200, 40, 32);
  } else if (personPosition == 2) {
    jiaoORbujiao->setGeometry(960, 200, 40, 32);
  }
  jiaoORbujiao->show();
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
    cardLabel->setGeometry(40 + 50 * i, 600, 360, 200);
    cardLabel->show();
  }
}

void PlayerA::displayCommonCards(const QList<int>& commonCards) {
  for (int i = 0; i < commonCards.size(); ++i) {
    CardLabel* cardLabel = new CardLabel(this);
    QPixmap cardImage =
        QPixmap("./assets/" + QString::number(commonCards[i]) + ".png");
    cardImage = cardImage.scaled(QSize(330, 160), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    cardLabel->setPixmap(cardImage);
    cardLabel->setGeometry(480 + 50 * i, 30, 360, 200);
    cardLabel->show();
  }
}

void PlayerA::afterAllHaveCalled() {
  qDebug() << "After all have called";
  for (auto plb : jiaoORbujiaoLabels_) {
    plb->hide();
  }
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
  sleep(50);

  for (int i = 0; i < 3; ++i) {
    cardsNum_.append(17);
  }
  cardsNum_[currentLandlord_] += 3;

  for (int i = 0; i < 3; ++i) {
    QLabel* numLabel = new QLabel(this);
    cardsNumLabel_.append(numLabel);
    cardsNumLabel_[i]->setText(QString::number(cardsNum_[i]));
  }
  cardsNumLabel_[0]->setGeometry(580, 500, 40, 20);
  cardsNumLabel_[1]->setGeometry(200, 100, 40, 20);
  cardsNumLabel_[2]->setGeometry(920, 100, 40, 20);
  cardsNumLabel_[0]->show();
  cardsNumLabel_[1]->show();
  cardsNumLabel_[2]->show();

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
  chuBtn->setGeometry(120, 500, 50, 50);
  buchuBtn->setGeometry(180, 500, 50, 50);
  chuBtn->show();
  buchuBtn->show();

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

      updateCardNumber(0, cardsToPush.size());

      chuBtn->hide();
      buchuBtn->hide();

      // A不再展示出掉的牌
      for (int i = 0; i < cardsToPush.size(); ++i) {
        cardsOfA_.removeOne(cardsToPush[i]);
      }
      displayCards();

      lastPushCardPerson_ = 0;

      // // A出完牌了
      // if (cardsOfA_.isEmpty()) {
      //   landlordWins_ = (currentLandlord_ == 0);
      //   sleep(50);
      //   castToB("gameOver",
      //           (landlordWins_ ^ (currentLandlord_ != 1)) ? "true" :
      //           "false");
      //   castToC("gameOver",
      //           (landlordWins_ ^ (currentLandlord_ != 2)) ? "true" :
      //           "false");
      //   showWinOrLoseInfo();
      //   showRestartOrExitBtnsOnSelfScreen();
      //   return;
      // }
      AHasJustPushedCards(cardsToPush);

      sleep(50);

      if (checkIfGameOver()) return;

      castToB("chuOrBuchu", "info");
    }
  });
  connect(buchuBtn, &QPushButton::clicked, [=]() {
    chuBtn->hide();
    buchuBtn->hide();
    giveUp(0);
  });
}

void PlayerA::AHasJustPushedCards(QList<int> cardsToPush) {
  cardsOnTable_ = cardsToPush;
  showTableOnSelfScreen(cardsOnTable_);
  cardsOnTable_.append(0);
  sleep(50);
  boardCast("someOneHasPushedCards", intArrayToString(cardsOnTable_));
}

void PlayerA::showTableOnSelfScreen(const QList<int>&) {
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

  if (giveupInfoLabels_.size() < 3) {
    for (int i = 0; i < 3; ++i) {
      QLabel* giveupLabel = new QLabel(this);
      giveupLabel->setText("不出");
      giveupInfoLabels_.append(giveupLabel);
    }
  }

  if (personPosition == 0) {
    giveupInfoLabels_[personPosition]->setGeometry(580, 400, 40, 32);
  } else if (personPosition == 1) {
    giveupInfoLabels_[personPosition]->setGeometry(160, 200, 40, 32);
  } else if (personPosition == 2) {
    giveupInfoLabels_[personPosition]->setGeometry(960, 200, 40, 32);
  }

  giveupInfoLabels_[personPosition]->show();
}

void PlayerA::showRestartOrExitBtnsOnSelfScreen() {
  QPushButton* restartBtn = new QPushButton(this);
  QPushButton* exitBtn = new QPushButton(this);

  restartBtn->setText("重新开始");
  exitBtn->setText("退出");
  restartBtn->setGeometry(970, 500, 50, 50);
  exitBtn->setGeometry(1030, 500, 50, 50);
  restartBtn->show();
  exitBtn->show();

  connect(exitBtn, SIGNAL(clicked()), qApp, SLOT(exit()));

  connect(restartBtn, &QPushButton::clicked, [=]() {
    // 等待另外两者连接
  });
}

void PlayerA::showWinOrLoseInfo() {
  QLabel* winOrLoseLabel = new QLabel(this);
  winOrLoseLabel->setText(
      currentLandlord_ != 0 ^ landlordWins_ ? "YOU WIN!!!" : "YOU LOSE!!!");
  winOrLoseLabel->setGeometry(200, 50, 800, 400);
  winOrLoseLabel->setFont(QFont("Helvetica", 48));
  winOrLoseLabel->show();
}

bool PlayerA::checkIfGameOver() {
  for (int i = 0; i < cardsNum_.size(); ++i) {
    if (cardsNum_[i] == 0) {
      landlordWins_ = (currentLandlord_ == i);
      showWinOrLoseInfo();
      showRestartOrExitBtnsOnSelfScreen();
      return true;
    }
  }
  return false;
}