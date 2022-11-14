// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>
#include <QLibrary>
#include <QProcess>

using RunFuncType = void*(*)(int argc, char* argv[]);
int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  QString libname("sharedlib_name_client");
  if (argc >= 2) {
    libname = "sharedlib_name_server";
  }
  QLibrary runlib(libname);
  RunFuncType run_fun = (RunFuncType)runlib.resolve("Run");
  QWidget* wgt = nullptr;
  if (run_fun) {
    wgt = static_cast<QWidget*>(run_fun(argc, argv));
  }
  auto ret = a.exec();
  return ret;
}
