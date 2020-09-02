#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonDocument>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QTime>
#include <QtNetwork>
// #include <QtTest>

#include "cardlabel.h"
#include "cards.h"

class PlayerA : public QMainWindow {
  Q_OBJECT

 public:
  PlayerA(QWidget *parent = nullptr);
  ~PlayerA();

  template <typename Data>
  void boardCast(Data data) {
    if (!socketAB_ || socketAC_) qDebug() << "Connection lost!";
    socketAB_->write(data);
    socketAC_->write(data);
  }

  template <typename Name, typename Value>
  void boardCast(Name str, Value v) {
    QJsonObject jsonObject;
    jsonObject.insert(str, v);

    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QByteArray dataArray = jsonDocument.toJson();

    socketAB_->write(dataArray);
    socketAC_->write(dataArray);
  }

  template <typename Name, typename Value>
  void castToB(Name str, Value v) {
    QJsonObject jsonObject;
    jsonObject.insert(str, v);

    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QByteArray dataArray = jsonDocument.toJson();

    socketAB_->write(dataArray);
  }

  template <typename QBA, typename Str>
  QString readFromBuffer(QBA buffer, Str key) {
    qDebug() << "buffer:" << buffer;
    // 转化为Json
    QJsonDocument document = QJsonDocument::fromJson(buffer);

    qDebug() << "document:" << document;

    QJsonObject rootObj = document.object();

    qDebug() << "rootObj:" << rootObj;

    return rootObj.value(key).toString();
  }

  template <typename Name, typename Value>
  void castToC(Name str, Value v) {
    QJsonObject jsonObject;
    jsonObject.insert(str, v);

    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QByteArray dataArray = jsonDocument.toJson();

    socketAC_->write(dataArray);
  }

  void firstCampaign(int n);

  void ifWantToCampaign(int, bool);  // send to b and c

  void startPlaying();

  void showChuOrBuchuBtns();

  void someOneHasJustPushedCards(QList<int>);

  void giveUp(int);

  void displayGiveUpInfo(int n);
  void askTheNextIfWantToChu(int n);

 private:
  // 图形界面
  QPushButton *buttonStartListening;
  // 连接
  QTcpServer *serverA_ = nullptr;
  QTcpSocket *socketAB_ = nullptr;
  QTcpSocket *socketAC_ = nullptr;
  const int portA_ = 10080;

  int personIndexToPosition_[3] = {0, 1, 2};

  QList<int> cardsOfA_;

  QList<int> commonCards_;

  QList<int> cardsOnTable_;

  QList<CardLabel *> cardLabels_;
  QList<CardLabel *> tableCardLabels_;

  // 出牌顺序是...->A->B->C->A->...
  // 地主为A：0，B：1，C：2
  int currentLandlord_;
  int currentCallNumber_ = 0;

  void sleep(int t) {
    QTime time;
    time.start();
    while (time.elapsed() < t)            //等待时间流逝50ms
      QCoreApplication::processEvents();  //不停地处理事件，让程序保持响应
  }

  QList<int> stringToIntArray(QString str) {
    QList<int> l;
    for (QString str : str.split(".")) {
      if (!str.isEmpty()) {
        l.append(str.toInt());
      }
    }
    return l;
  }
  QString intArrayToString(QList<int> list) {
    QString str;
    for (int i = 0; i < list.size(); ++i) {
      str.push_back(QString::number(list[i]));
      str.push_back('.');
    }
    return str;
  }

  void displayCards();
  void displayCommonCards(QList<int>);

  void showTableOnSelfScreen(QList<int>);

  int lastPushCardPerson_ = 0;

 public slots:
  void startListening();

  void handleConnections();
  void socketReadDataFromB();
  void socketReadDataFromC();
  void socketDisconnectedFromB();
  void socketDisconnectedFromC();

  void startCampaign();

  void askIfCallLandlord(int n);

  void showOthersIfCampaignInfo(int, bool);

  void afterAllHaveCalled();

  // void pushCards();

 signals:
  // 所有人已经叫过地主了
  void allHaveCalled();  // send to b and c

  void nextOneToCampaign();
};
#endif  // MAINWINDOW_H
