// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

#include "transcoder/transcoder_export.h"
#include "transcoder_base/log/log_writer.h"
#include "transcoder_client_frame.h"

extern "C" {

TRANSCODER_API int Run(int argc, char* argv[]) {
  QApplication a(argc, argv);
  QString app_data_dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString log_dir = QString("%1/log").arg(app_data_dir);
  QString log_path =
      QString("%1/transcoder_client_%2.log").arg(log_dir).arg(curr_time_str);
  QDir log_qdir(log_dir);
  if (!log_qdir.exists()) {
    log_qdir.mkpath(".");
  }
  TRANSCODER_BASE::InitLog(log_path.toStdString().c_str());
  log_info << "transcoder_client start:";
  QObject::connect(&a, &QApplication::aboutToQuit, []() {
    // exit app
    log_info << "transcoder_client about to quit!";
  });
  TranscoderClientFrame w;
  w.show();
  auto ret = a.exec();
  TRANSCODER_BASE::UnInitLog();
  return ret;
}
}
