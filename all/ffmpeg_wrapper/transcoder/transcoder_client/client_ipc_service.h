// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDataStream>
#include <QLocalSocket>

class ClientIpcService : public QObject {
  Q_OBJECT

 public:
  explicit ClientIpcService(QObject* parent = nullptr);

  void ConnectToServer();
  void Write(const QString& data) { WriteData(data); }

signals:
  void DataChanged(const QString& data);

 private slots:
  void OnReadyRead();
  void OnErrorOccurred(QLocalSocket::LocalSocketError socketError);
  void OnSocketStateChanged(
      QLocalSocket::LocalSocketState socket_state);

 private:
  void WriteData(const QString& data);

 private:
  QLocalSocket* socket_{nullptr};
  QLocalSocket::LocalSocketState socket_state_{QLocalSocket::UnconnectedState};
  QDataStream socket_stream_;
  QString curr_data_;
};
