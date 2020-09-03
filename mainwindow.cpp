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

  initGiveUpInfoLabels();

  initDizhuNongminLabels();

  initBuhefaLabel();

  // Game started
}

void PlayerA::restart() {
  delete winOrLoseLabel_;
  delete restartBtn_;
  delete exitBtn_;

  cardsOfA_.clear();
  commonCards_.clear();
  for (auto* i : commonCardLabels_) i->hide();
  commonCardLabels_.clear();
  cardsOnTable_.clear();
  for (auto* i : cardLabels_) i->hide();
  cardLabels_.clear();
  for (auto* i : tableCardLabels_) i->hide();
  tableCardLabels_.clear();
  jiaoORbujiaoLabels_.clear();

  currentLandlord_ = 0;
  currentCallNumber_ = 0;
  cardsNum_.clear();
  for (auto* i : cardsNumLabel_) i->hide();
  cardsNumLabel_.clear();

  for (auto* i : dizhuNongminLabels_) i->hide();
  for (auto* i : giveupInfoLabels_) i->hide();

  if (BWantsToRestart_ && CWantsToRestart_)
    startCampaign();
  else
    connect(this, &PlayerA::BCBothWantToRestart, [=]() { startCampaign(); });
}

void PlayerA::initGiveUpInfoLabels() {
  for (int i = 0; i < 3; ++i) {
    QLabel* giveupLabel = new QLabel(this);
    giveupLabel->setText("不出");
    giveupInfoLabels_.append(giveupLabel);
    giveupInfoLabels_[i]->hide();
  }

  giveupInfoLabels_[0]->setGeometry(580, 440, 40, 32);
  giveupInfoLabels_[1]->setGeometry(160, 200, 40, 32);
  giveupInfoLabels_[2]->setGeometry(960, 200, 40, 32);
}

void PlayerA::initDizhuNongminLabels() {
  for (int i = 0; i < 3; ++i) {
    QLabel* dizhuNongminLabel = new QLabel(this);
    dizhuNongminLabel->setText("身份不明");
    dizhuNongminLabels_.append(dizhuNongminLabel);
    dizhuNongminLabels_[i]->hide();
  }

  dizhuNongminLabels_[0]->setGeometry(580, 420, 40, 32);
  dizhuNongminLabels_[1]->setGeometry(160, 168, 40, 32);
  dizhuNongminLabels_[2]->setGeometry(960, 168, 40, 32);
}

void PlayerA::initBuhefaLabel() {
  buhefaLabel_ = new QLabel(this);
  buhefaLabel_->hide();
  buhefaLabel_->setText("不合法");
  buhefaLabel_->setGeometry(240, 500, 50, 50);
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
    giveupInfoLabels_[personIndexToPosition_[1]]->hide();

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
    giveupInfoLabels_[personIndexToPosition_[1]]->show();
    giveUp(1);
  }

  QString restart = readFromBuffer(buffer, "restart");
  if (!restart.isEmpty()) {
    BWantsToRestart_ = true;
    qDebug() << "BWantsToRestart_ = true;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;";
    if (CWantsToRestart_) emit BCBothWantToRestart();
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
    giveupInfoLabels_[personIndexToPosition_[2]]->hide();

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
    giveupInfoLabels_[personIndexToPosition_[2]]->show();
    giveUp(2);
  }

  QString restart = readFromBuffer(buffer, "restart");
  if (!restart.isEmpty()) {
    CWantsToRestart_ = true;
    qDebug() << "CWantsToRestart_ = true;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;";
    if (BWantsToRestart_) emit BCBothWantToRestart();
  }
}
void PlayerA::socketDisconnectedFromB() {}
void PlayerA::socketDisconnectedFromC() {}

void PlayerA::startCampaign() {
  BWantsToRestart_ = false;
  CWantsToRestart_ = false;

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

    yesBtn->setText("叫地主");
    noBtn->setText("不叫");
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
    commonCardLabels_.append(cardLabel);
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
  for (int i = 0; i < 3; ++i) {
    dizhuNongminLabels_[personIndexToPosition_[i]]->setText(
        currentLandlord_ == i ? "地主" : "农民");
    dizhuNongminLabels_[personIndexToPosition_[i]]->show();
  }
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
      giveupInfoLabels_[personIndexToPosition_[0]]->hide();

      buhefaLabel_->hide();
      updateCardNumber(0, cardsToPush.size());

      chuBtn->hide();
      buchuBtn->hide();

      // A不再展示出掉的牌
      for (int i = 0; i < cardsToPush.size(); ++i) {
        cardsOfA_.removeOne(cardsToPush[i]);
      }
      displayCards();

      lastPushCardPerson_ = 0;

      AHasJustPushedCards(cardsToPush);

      sleep(50);

      if (checkIfGameOver()) return;

      castToB("chuOrBuchu", "info");
    } else {
      buhefaLabel_->show();
    }
  });
  connect(buchuBtn, &QPushButton::clicked, [=]() {
    buhefaLabel_->hide();
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
  giveupInfoLabels_[personIndexToPosition_[n]]->show();
  boardCast("someOneHasJustGivenUp", QString::number(n));
  sleep(50);
  if (n == 0)
    castToB("chuOrBuchu", "info");
  else if (n == 1)
    castToC("chuOrBuchu", "info");
  else if (n == 2)
    showChuOrBuchuBtns();
}

void PlayerA::showRestartOrExitBtnsOnSelfScreen() {
  restartBtn_ = new QPushButton(this);
  exitBtn_ = new QPushButton(this);

  restartBtn_->setText("重新开始");
  exitBtn_->setText("退出");
  restartBtn_->setGeometry(970, 500, 50, 50);
  exitBtn_->setGeometry(1030, 500, 50, 50);
  restartBtn_->show();
  exitBtn_->show();

  connect(exitBtn_, SIGNAL(clicked()), QApplication::instance(), SLOT(quit()));

  connect(restartBtn_, &QPushButton::clicked, [=]() { restart(); });
}

void PlayerA::showWinOrLoseInfo() {
  winOrLoseLabel_ = new QLabel(this);
  winOrLoseLabel_->setText(
      currentLandlord_ != 0 ^ landlordWins_ ? "YOU WIN!!!" : "YOU LOSE!!!");
  winOrLoseLabel_->setGeometry(200, 50, 800, 400);
  winOrLoseLabel_->setFont(QFont("Helvetica", 48));
  winOrLoseLabel_->show();
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