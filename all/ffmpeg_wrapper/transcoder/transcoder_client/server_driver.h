// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QApplication>
#include <QDebug>
#include <QProcess>

class ServerDriver : public QObject {
  Q_OBJECT

 public:
  explicit ServerDriver(QObject* parent = nullptr);
  ~ServerDriver();

  void StartServer(QStringList ffmpeg_args);

  void StopServer();

  bool IsServerRunning() const {
    if (server_) {
      return true;
    }
    return false;
  }

 signals:
  void ServerStarted();
  void ServerFinished(bool normal);

 private slots:
  void OnChildStarted();
  void OnChildFinished(int exit_code, QProcess::ExitStatus exit_status);

 private:
  QProcess* server_{nullptr};
};
