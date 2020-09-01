#include "mainwindow.h"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QTime>
#include <algorithm>

PlayerA::PlayerA(QWidget* parent) : QMainWindow(parent) {
  setFixedSize(1200, 800);

  // startListening button
  buttonStartListening = new QPushButton(this);
  buttonStartListening->setGeometry(376, 512, 448, 120);
  buttonStartListening->setFont(QFont("Helvetica", 28));
  buttonStartListening->setText("Start connecting...");
  connect(buttonStartListening, SIGNAL(clicked()), this,
          SLOT(startListening()));

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
void PlayerA::socketReadDataFromB() {}
void PlayerA::socketReadDataFromC() {}
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
  std::random_shuffle(cards_index.begin(), cards_index.end());

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

  n = 0;

  if (n == 0) {
    //自己
    askIfCallLandlord(0);
  } else if (n == 1) {
    // B
    askIfCallLandlord(1);

  } else if (n == 2) {
    // C
    askIfCallLandlord(2);
  }
}

void PlayerA::askIfCallLandlord(int n) {
  qDebug() << "ASKIFCALLLANDLORD" << n;
  // currentLandlord_ = n;

  // 检查是否所有人都叫过地主了
  ++currentCallNumber_;
  if (currentCallNumber_ == 3) {
    emit allHaveCalled();
    return;
  }

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

    connect(yesBtn, &QPushButton::clicked, [=]() { askIfCallLandlord(1); });
    connect(noBtn, &QPushButton::clicked, [=]() { askIfCallLandlord(1); });
  }
}

void PlayerA::ifWantToCampaign(int n, bool b) {
  if (b) {
    boardCast("wantToCampaign", n);
  } else {
    boardCast("notWantToCampaign", n);
  }

  if (n == 0) {
    showSelfIfCampaignInfo(b);
  }
}

void PlayerA::showSelfIfCampaignInfo(bool b) {
  QLabel* jiaoORbujiao = new QLabel(this);
  jiaoORbujiao->setText(b ? "叫" : "不叫");
  jiaoORbujiao->setGeometry(220, 220, 50, 50);
  jiaoORbujiao->show();
}