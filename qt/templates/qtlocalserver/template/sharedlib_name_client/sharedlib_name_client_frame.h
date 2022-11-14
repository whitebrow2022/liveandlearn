// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SharedlibNameClientFrame;
}
QT_END_NAMESPACE

class ServerDriver;
class ClientIpcService;

class SharedlibNameClientFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit SharedlibNameClientFrame(QWidget *parent = nullptr);
  ~SharedlibNameClientFrame();

 protected:
  void closeEvent(QCloseEvent *event) override;

 private slots:
  void OnCreate();
  void OnConnectToServer();
  void Exit();
  void About();
  void OnClickedAndSend();

  void OnServerDataChanged(const QString &data);

 private:
  Ui::SharedlibNameClientFrame *ui_{nullptr};
  ServerDriver *driver_{nullptr};
  ClientIpcService *client_{nullptr};
};
