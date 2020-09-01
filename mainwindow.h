#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonDocument>
#include <QMainWindow>
#include <QPushButton>
#include <QtNetwork>

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

 private:
  // 图形界面
  QPushButton *buttonStartListening;
  // 连接
  // bool Bconnected_ = false;
  // bool Cconnected_ = false;
  QTcpServer *serverA_ = nullptr;
  QTcpSocket *socketAB_ = nullptr;
  QTcpSocket *socketAC_ = nullptr;
  const int portA_ = 10080;

  // 出牌顺序是...->A->B->C->A->...
  // 地主为A：0，B：1，C：2
  int currentLandlord_;
  int currentCallNumber_ = 0;

 public slots:
  void startListening();

  void handleConnections();
  void socketReadDataFromB();
  void socketReadDataFromC();
  void socketDisconnectedFromB();
  void socketDisconnectedFromC();

  void startCampaign();

  void askIfCallLandlord(int n);

  void showSelfIfCampaignInfo(bool);

 signals:
  // 所有人已经叫过地主了
  void allHaveCalled();  // send to b and c

  void nextOneToCampaign();
};
#endif  // MAINWINDOW_H
