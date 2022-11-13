// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusServer Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QBrush>
#include <QGraphicsObject>

class Car : public QGraphicsObject {
  Q_OBJECT
 public:
  Car();
  QRectF boundingRect() const;

 public Q_SLOTS:
  void accelerate();
  void decelerate();
  void turnLeft();
  void turnRight();

 Q_SIGNALS:
  void crashed();

 protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget = 0);
  void timerEvent(QTimerEvent *event);

 private:
  QBrush color;
  qreal wheelsAngle;  // used when applying rotation
  qreal speed;        // delta movement along the body axis
};
