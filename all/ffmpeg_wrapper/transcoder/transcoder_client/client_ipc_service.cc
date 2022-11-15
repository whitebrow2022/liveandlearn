// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "client_ipc_service.h"

#include <QDateTime>
#include <QMessageBox>
#include <QTimer>

ClientIpcService::ClientIpcService(QObject* parent /* = nullptr*/)
    : QObject(parent), socket_(nullptr) {}

const QString& ClientIpcService::GetServerName() const {
  static QString server_name =
      QString("TranscoderUserServer-%1")
          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
  return server_name;
}

bool ClientIpcService::StartServer() {
  qDebug() << __FUNCTION__;
  if (server_) {
    return true;
  }
  server_ = new QLocalServer(this);
  if (!server_->listen(GetServerName())) {
    QMessageBox::critical(
        nullptr, tr("Local Fortune Server"),
        tr("Unable to start the server: %1.").arg(server_->errorString()));
    return false;
  }
  connect(server_, &QLocalServer::newConnection, this,
          &ClientIpcService::OnNewConnection);
  return true;
}

void ClientIpcService::OnNewConnection() {
  socket_ = server_->nextPendingConnection();
  // server_->close();  // because only listen one

  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);
  connect(socket_, &QLocalSocket::readyRead, this,
          &ClientIpcService::OnReadyRead);
  connect(socket_, &QLocalSocket::errorOccurred, this,
          &ClientIpcService::OnErrorOccurred);
  connect(socket_, &QLocalSocket::stateChanged, this,
          &ClientIpcService::OnSocketStateChanged);
  connect(socket_, &QLocalSocket::disconnected, this,
          &ClientIpcService::OnSocketDisconnected);
  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);

  emit(TranscoderConnected());
}

void ClientIpcService::OnReadyRead() {
  if (!socket_) {
    return;
  }
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
  // TODO: fix localsocket loss data
  QTimer::singleShot(0, this, &ClientIpcService::OnReadyRead);
}
void ClientIpcService::OnErrorOccurred(
    QLocalSocket::LocalSocketError socketError) {}

void ClientIpcService::OnSocketStateChanged(
    QLocalSocket::LocalSocketState socket_state) {
  qInfo() << "socket_state: " << socket_state;
  socket_state_ = socket_state;
}

void ClientIpcService::OnSocketDisconnected() {
  socket_->deleteLater();
  socket_ = nullptr;
  emit(TranscoderDisconnected());
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
