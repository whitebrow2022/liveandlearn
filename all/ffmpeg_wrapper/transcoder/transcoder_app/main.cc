// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QLibrary>
#include <QProcess>
#include <QSettings>
#include <QTranslator>
#include <QWidget>

using RunFuncType = int (*)(int argc, char* argv[]);
int main(int argc, char* argv[]) {
#if 0
  QApplication a(argc, argv);
  QWidget w;
  w.show();
  return a.exec();
#else
#if defined(Q_OS_MACOS)
  QString libname;
  {
    QApplication a(argc, argv);
    qInfo() << "app: " << QCoreApplication::applicationDirPath();
    QString framework_dir =
        QString("%1/../Frameworks").arg(QCoreApplication::applicationDirPath());
    libname = QString("%1/libtranscoder_client.dylib").arg(framework_dir);
    if (argc >= 2) {
      libname = QString("%1/libtranscoder_server.dylib").arg(framework_dir);
    }
  }
#else
  QString libname("transcoder_client");
  if (argc >= 2) {
    libname = "transcoder_server";
  }
#endif
  qInfo() << "libanme: " << libname;
  QLibrary runlib(libname);
  RunFuncType run_fun = (RunFuncType)runlib.resolve("Run");
  if (!run_fun) {
    return 0;
  }
  return run_fun(argc, argv);
#endif
}
