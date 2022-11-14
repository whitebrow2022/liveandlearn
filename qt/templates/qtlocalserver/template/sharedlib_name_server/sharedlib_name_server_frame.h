// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SharedlibNameServerFrame;
}
QT_END_NAMESPACE

class ServerIpcService;
class FakeProgressTask;
class SharedlibNameServerFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit SharedlibNameServerFrame(QWidget *parent = nullptr);
  ~SharedlibNameServerFrame();

 protected:
  void showEvent(QShowEvent *event) override;

 private slots:
  void OnCreate();
  void Exit();
  void About();

  void OnClientDisconnected();
  void OnClientDataChanged(const QString &data);

  void OnTaskStarted();
  void OnTaskProgressed(double progress);
  void OnTaskStoped();

 private:
  Ui::SharedlibNameServerFrame *ui_{nullptr};
  ServerIpcService *service_{nullptr};
  FakeProgressTask *task_{nullptr};
};
