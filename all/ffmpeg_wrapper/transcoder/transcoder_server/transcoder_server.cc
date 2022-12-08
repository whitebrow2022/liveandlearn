// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>
#include <QDir>

#include "server_ipc_service.h"
#include "transcoder/transcoder_export.h"
#include "transcoder_base/log/log_writer.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "ffmpeg/ffmpeg.h"
#ifdef __cplusplus
}
#endif

#ifdef WIN32
#include <Windows.h>
#endif

namespace {
struct ArgWrapper {
  explicit ArgWrapper(const QStringList qstrlist) {
    argc = qstrlist.size();
    argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
      auto buflen = qstrlist[i].toUtf8().size() + 1;
      argv[i] = new char[buflen];
      snprintf(argv[i], buflen, "%s", qstrlist[i].toUtf8().data());
    }
  }
  ~ArgWrapper() {
    // TODO(liangxu): release memory.
  }

  char** argv{nullptr};
  int argc{0};
};
}  // namespace

extern "C" {

TRANSCODER_API int Run(int argc, char* argv[]) {
#ifdef WIN32
  // while (!::IsDebuggerPresent()) {
  //  ::Sleep(100);  // to avoid 100% CPU load
  //}
#endif
#if 0
  QApplication a(argc, argv);
  TranscoderServerFrame w;
  w.show();
  auto ret = a.exec();
  return ret;
#else
  QApplication a(argc, argv);

  // init log
  QString app_data_dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString log_dir = QString("%1/log").arg(app_data_dir);
  QString log_path =
      QString("%1/transcoder_server_%2.log").arg(log_dir).arg(curr_time_str);
  QDir log_qdir(log_dir);
  if (!log_qdir.exists()) {
    log_qdir.mkpath(".");
  }
  TRANSCODER_BASE::InitLog(log_path.toStdString().c_str());
  log_info << "transcoder_server start:";
  QObject::connect(&a, &QApplication::aboutToQuit, []() {
    // exit app
    log_info << "transcoder_server about to quit!";
  });
  if (QApplication::arguments().size() > 1) {
    auto server_name = QApplication::arguments()[1];
    a.connect(&ServerIpcService::GetInstance(),
              &ServerIpcService::TranscoderReady, []() {
                QStringList ffmpeg_args = QApplication::arguments();
                ffmpeg_args.removeAt(1);
                qInfo() << "ffmpeg args: " << ffmpeg_args;
                ArgWrapper args(ffmpeg_args);
                ffmpeg_main(args.argc, args.argv);
              });
    ServerIpcService::GetInstance().ConnectToServer(server_name);
  } else {
    return 1;
  }
  auto ret = a.exec();
  TRANSCODER_BASE::UnInitLog();
  return ret;
#endif
}
}
