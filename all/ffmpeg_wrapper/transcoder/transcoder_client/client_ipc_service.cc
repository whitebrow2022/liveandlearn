// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "client_ipc_service.h"

ClientIpcService::ClientIpcService(QObject* parent /* = nullptr*/)
    : QObject(parent), socket_(new QLocalSocket(this)) {

  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);
  connect(socket_, &QLocalSocket::readyRead, this,
          &ClientIpcService::OnReadyRead);
  connect(socket_, &QLocalSocket::errorOccurred, this,
          &ClientIpcService::OnErrorOccurred);
  connect(socket_, &QLocalSocket::stateChanged, this,
          &ClientIpcService::OnSocketStateChanged);
}

void ClientIpcService::ConnectToServer() {
  if (!socket_) {
    return;
  }

  if (socket_state_ == QLocalSocket::UnconnectedState) {
    socket_->connectToServer("TranscoderServer");
  } else if (socket_state_ == QLocalSocket::ConnectedState) {
    static quint64 written_count = 0;
    QString message =
        QString("ConnectToServer Written Number: %1").arg(written_count++);
    WriteData(message);
  }
}

void ClientIpcService::OnReadyRead() {
  quint32 block_size = 0;
  // Relies on the fact that QDataStream serializes a quint32 into
  // sizeof(quint32) bytes
  if (socket_->bytesAvailable() < static_cast<int>(sizeof(quint32))) {
    return;
  }
  socket_stream_ >> block_size;

  if (socket_->bytesAvailable() < block_size || socket_stream_.atEnd()) {
    return;
  }

  QString server_data;
  socket_stream_ >> server_data;

  emit(DataChanged(server_data));

  // retry read more data
  OnReadyRead();
}
void ClientIpcService::OnErrorOccurred(
  QLocalSocket::LocalSocketError socketError) {
}

void ClientIpcService::OnSocketStateChanged(
    QLocalSocket::LocalSocketState socket_state) {
  socket_state_ = socket_state;
}

void ClientIpcService::WriteData(const QString& data) {
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_10);
  QString message = data;
  out << quint32(message.size());
  out << message;
  socket_->write(block);
  socket_->flush();
}
