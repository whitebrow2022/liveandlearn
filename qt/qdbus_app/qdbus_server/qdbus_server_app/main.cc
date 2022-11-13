// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusServer Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QtDBus/QDBusConnection>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>

#include "car.h"
#include "car_adaptor.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WINDOWS
    // This variable comes from dbus-env.bat
    qputenv("DBUS_SESSION_BUS_ADDRESS", "autolaunch:");
#endif
  QApplication app(argc, argv);

  QGraphicsScene scene;
  scene.setSceneRect(-500, -500, 1000, 1000);
  scene.setItemIndexMethod(QGraphicsScene::NoIndex);

  Car *car = new Car();
  scene.addItem(car);

  QGraphicsView view(&scene);
  view.setRenderHint(QPainter::Antialiasing);
  view.setBackgroundBrush(Qt::darkGray);
  view.setWindowTitle(
      QT_TRANSLATE_NOOP(QGraphicsView, "Qt DBus Controlled Car"));
  view.resize(400, 300);
  view.show();

  new CarInterfaceAdaptor(car);
  QDBusConnection connection = QDBusConnection::sessionBus();
  connection.unregisterObject("/Car");
  bool ret = connection.registerObject("/Car", car);
  if (!ret) {
      qDebug() << connection.lastError().message();
  }
  ret = connection.registerService("org.example.CarExample");
  if (!ret) {
      qDebug() << connection.lastError().message();
  }
  if (!ret) {
      return 0;
  }

  return app.exec();
}
