﻿// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

#include "hash_generator_base/log/log_writer.h"
#include "hash_generator_frame.h"

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
                  app_path + "/../../hash_generator_app_" + locale);
  {
    LoadTranslation(&translator[1],
                    app_path + "/../hash_generator_app_" + locale);
    LoadTranslation(&translator[2], app_path + "/hash_generator_app_" + locale);
  }
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  QString app_data_dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString log_dir = QString("%1/log").arg(app_data_dir);
  QString log_path =
      QString("%1/sharedlib_name_app_%2.log").arg(log_dir).arg(curr_time_str);
  QDir log_qdir(log_dir);
  if (!log_qdir.exists()) {
    log_qdir.mkpath(".");
  }
  HASH_GENERATOR_BASE::InitLog(log_path.toStdString().c_str());
  log_info << "hash_generator_app start:";
  // multiple langs
  // QTranslator translator[3];
  // LoadTranslations(translator);

  HashGeneratorFrame w;
  QObject::connect(&a, &QApplication::aboutToQuit, []() {
    // exit app
    log_info << "hash_generator_app about to quit!";
  });
  w.show();
  auto ret = a.exec();
  HASH_GENERATOR_BASE::UnInitLog();
  return ret;
}
