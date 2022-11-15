// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDataStream>
#include <QLocalSocket>
#include <QLocalServer>

class ClientIpcService : public QObject {
  Q_OBJECT

 public:
  explicit ClientIpcService(QObject* parent = nullptr);

  const QString& GetServerName() const;
  bool StartServer();
  void Write(const QString& data) { WriteData(data); }

signals:
  void TranscoderConnected();
  void TranscoderDisconnected();
  void DataChanged(const QString& data);

 private slots:
  void OnNewConnection();
  void OnReadyRead();
  void OnErrorOccurred(QLocalSocket::LocalSocketError socketError);
  void OnSocketStateChanged(QLocalSocket::LocalSocketState socket_state);
  void OnSocketDisconnected();

 private:
  void WriteData(const QString& data);

 private:
  QLocalServer* server_{nullptr};
  QLocalSocket* socket_{nullptr};
  QLocalSocket::LocalSocketState socket_state_{QLocalSocket::UnconnectedState};
  QDataStream socket_stream_;
};
