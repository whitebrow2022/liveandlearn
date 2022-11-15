// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include "server_ipc_service.h"
#include "transcoder/transcoder_export.h"

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
      argv[i] = new char[qstrlist[i].toUtf8().size() + 1];
      strcpy(argv[i], qstrlist[i].toUtf8().data());
    }
  }
  ~ArgWrapper() {
    // TODO: release memory
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
  return ret;
#endif
}
}
