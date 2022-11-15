// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>

class ServerIpcService : public QObject {
  Q_OBJECT

 public:
  ~ServerIpcService() = default;
  static ServerIpcService& GetInstance();

 private:
  explicit ServerIpcService(QObject* parent = nullptr);

 public:
  void ConnectToServer(const QString& server_name);
  void Write(const QString& data) { this->WriteData(data); }

 signals:
  void TranscoderReady();
  void DataChanged(const QString& data);

 private slots:
  void OnReadyRead();
  void OnErrorOccurred(QLocalSocket::LocalSocketError socketError);
  void OnSocketStateChanged(QLocalSocket::LocalSocketState socket_state);
  void OnSocketConnected();
  void OnSocketDisconnected();

 private:
  void WriteData(const QString& data);

 private:
  QLocalSocket* socket_{nullptr};
  QLocalSocket::LocalSocketState socket_state_{QLocalSocket::UnconnectedState};
  QDataStream socket_stream_;
};
