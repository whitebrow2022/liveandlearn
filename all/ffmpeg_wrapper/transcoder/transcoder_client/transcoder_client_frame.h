// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>
#include <chrono>  // NOLINT


QT_BEGIN_NAMESPACE
namespace Ui {
class TranscoderClientFrame;
}
QT_END_NAMESPACE

class ServerDriver;
class ClientIpcService;

class TranscoderClientFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit TranscoderClientFrame(QWidget *parent = nullptr);
  ~TranscoderClientFrame();

 protected:
  void closeEvent(QCloseEvent *event) override;

 private slots:
  void OnTranscode();
  void Exit();
  void About();
  void OnClickedAndSend();

  void OnServerDataChanged(const QString &data);

 private:
  void StartServer(const QStringList &command_list);
  QString GetSourceVideoPath() const;

 private:
  Ui::TranscoderClientFrame *ui_{nullptr};
  ServerDriver *driver_{nullptr};
  ClientIpcService *client_{nullptr};

  QString output_path_;
  QString last_input_file_;
  std::chrono::time_point<std::chrono::high_resolution_clock>
      transcode_time_start_;
};
