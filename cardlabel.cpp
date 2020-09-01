#include "cardlabel.h"

CardLabel::CardLabel(QWidget* parent) : QLabel(parent) {}

void CardLabel::mousePressEvent(QMouseEvent* event) {
  Q_UNUSED(event);

  if (!moved_)
    this->setGeometry(this->x(), this->y() - 40, this->width(), this->height());
  else
    this->setGeometry(this->x(), this->y() + 40, this->width(), this->height());

  moved_ = !moved_;
}