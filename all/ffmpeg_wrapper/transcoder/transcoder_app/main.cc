// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>
#include <QLibrary>
#include <QProcess>

using RunFuncType = int(*)(int argc, char* argv[]);
int main(int argc, char* argv[]) {
  QString libname("transcoder_client");
  if (argc >= 2) {
    libname = "transcoder_server";
  }
  QLibrary runlib(libname);
  RunFuncType run_fun = (RunFuncType)runlib.resolve("Run");
  if (!run_fun) {
    return 0;
  }
  return run_fun(argc, argv);
}
