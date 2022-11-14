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
#include "transcoder_client_frame.h"

extern "C" {

TRANSCODER_API  int Run(int argc, char* argv[]) {
  QApplication a(argc, argv);
  TranscoderClientFrame w;
  w.show();
  auto ret = a.exec();
  return ret;
}

}
