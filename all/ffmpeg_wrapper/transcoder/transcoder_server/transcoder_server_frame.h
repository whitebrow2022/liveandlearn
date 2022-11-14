// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class TranscoderServerFrame;
}
QT_END_NAMESPACE

class ServerIpcService;
class FakeProgressTask;
class TranscoderServerFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit TranscoderServerFrame(QWidget *parent = nullptr);
  ~TranscoderServerFrame();

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
  Ui::TranscoderServerFrame *ui_{nullptr};
  ServerIpcService *service_{nullptr};
  FakeProgressTask *task_{nullptr};
};
