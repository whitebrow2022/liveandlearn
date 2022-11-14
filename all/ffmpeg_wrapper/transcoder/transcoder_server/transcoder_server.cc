// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include "transcoder/transcoder_export.h"
#include "transcoder_server_frame.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "ffmpeg/ffmpeg.h"
#ifdef __cplusplus
}
#endif

extern "C" {

TRANSCODER_API int Run(int argc, char* argv[]) {
#ifdef WIN32
  while (!::IsDebuggerPresent()) {
    ::Sleep(100);  // to avoid 100% CPU load
  }
#endif
#if 0
  QApplication a(argc, argv);
  TranscoderServerFrame w;
  w.show();
  auto ret = a.exec();
  return ret;
#else
  qDebug() << argv;
  return ffmpeg_main(argc, argv);
#endif
}
}
