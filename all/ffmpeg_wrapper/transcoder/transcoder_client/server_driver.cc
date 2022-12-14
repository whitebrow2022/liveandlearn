// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "server_driver.h"

ServerDriver::ServerDriver(QObject* parent /* = nullptr*/) : QObject(parent) {}

ServerDriver::~ServerDriver() {
  if (server_) {
    delete server_;
  }
}

void ServerDriver::StartServer(QStringList ffmpeg_args) {
  if (server_) {
    return;
  }
  QStringList arguments = ffmpeg_args;
  server_ = new QProcess(this);
  connect(server_,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ServerDriver::OnChildFinished);
  connect(server_, &QProcess::started, this, &ServerDriver::OnChildStarted);
  // server run in app shell
  server_->start(QApplication::applicationFilePath(), arguments);
}

void ServerDriver::StopServer() {
  if (server_) {
    server_->terminate();
  }
}

void ServerDriver::OnChildStarted() {
  qDebug() << QString("child process started");
  emit(ServerStarted());
}

void ServerDriver::OnChildFinished(int exit_code,
                                   QProcess::ExitStatus exit_status) {
  qDebug() << QString("child process exit(%1) with status(%2)")
                  .arg(exit_code)
                  .arg(exit_status);
  if (server_) {
    server_->deleteLater();
    server_ = nullptr;
  }

  emit(ServerFinished(exit_code == 0));
}
