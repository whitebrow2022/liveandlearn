// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusClient Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include "qdbus_client_frame.h"
#include "controller.h"

void LoadTranslation(QTranslator* translator, const QString& file_name,
                     const QString& directory = QString()) {
  bool is_loaded = translator->load(file_name, directory);
  bool is_installed = QCoreApplication::installTranslator(translator);
  qDebug() << "Translation " << file_name << " in " << directory
           << " is_loaded: " << is_loaded << ", is_installed: " << is_installed;
}

void LoadTranslations(QTranslator* translator) {
  QSettings settings;
  QString locale =
      settings.value(QStringLiteral("interfaceLanguage")).toString();

  if (locale.isEmpty()) {
    locale = QLocale::system().name().section('_', 0, 0);
  }
  QString app_path = QCoreApplication::applicationDirPath();
  LoadTranslation(&translator[0],
                  app_path + "/../../qdbus_client_app_" + locale);
  {
    LoadTranslation(&translator[1],
                    app_path + "/../qdbus_client_app_" + locale);
    LoadTranslation(&translator[2], app_path + "/qdbus_client_app_" + locale);
  }
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
#if 0
  // multiple langs
  // QTranslator translator[3];
  // LoadTranslations(translator);
  QdbusClientFrame w;
  w.show();
#else
  Controller controller;
  controller.show();
#endif
  return a.exec();
}
