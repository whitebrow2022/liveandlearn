// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDataStream>
#include <QLocalSocket>
#include <QLocalServer>

class ServerIpcService : public QObject {
  Q_OBJECT

 public:
  explicit ServerIpcService(QObject* parent = nullptr);

  bool StartServer();
  void Write(const QString& data) { this->WriteData(data); }

signals:
  void DataChanged(const QString& data);
  void ClientDisconnected();

 private slots:
  void OnNewConnection();
  void OnSocketDisconnected();
  void OnReadyRead();
  void OnErrorOccurred(QLocalSocket::LocalSocketError socketError);

 private:
  void WriteData(const QString& data);

 private:
  QLocalSocket* socket_{nullptr};
  QDataStream socket_stream_;
  QLocalServer* server_{nullptr};
};
