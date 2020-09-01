#include <QEvent>
#include <QLabel>
#include <QObject>

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