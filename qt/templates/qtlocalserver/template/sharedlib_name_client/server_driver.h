// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
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

  void StartServer();

  void StopServer();

  bool IsServerRunning() const {
    if (server_) {
      return true;
    }
    return false;
  }

 private slots:
  void OnChildFinished(int exit_code, QProcess::ExitStatus exit_status);

 private:
  QProcess* server_{nullptr};
};
