// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include "sharedlib_name_base/log/log_writer.h"
#include "sharedlib_name_frame.h"

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
                  app_path + "/../../sharedlib_name_app_" + locale);
  {
    LoadTranslation(&translator[1],
                    app_path + "/../sharedlib_name_app_" + locale);
    LoadTranslation(&translator[2], app_path + "/sharedlib_name_app_" + locale);
  }
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  log_info << "sharedlib_name_app start:";
  // multiple langs
  // QTranslator translator[3];
  // LoadTranslations(translator);

  SharedlibNameFrame w;
  QObject::connect(&a, &QApplication::aboutToQuit, []() {
    // exit app
    log_info << "sharedlib_name_app about to quit!";
  });
  w.show();
  return a.exec();
}
