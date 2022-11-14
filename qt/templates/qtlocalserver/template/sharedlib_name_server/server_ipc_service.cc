// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "server_ipc_service.h"

#include <QMessageBox>

ServerIpcService::ServerIpcService(QObject* parent /* = nullptr*/)
    : QObject(parent), socket_(new QLocalSocket(this)) {
  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);
  connect(socket_, &QLocalSocket::readyRead, this,
          &ServerIpcService::OnReadyRead);
  connect(socket_, &QLocalSocket::errorOccurred, this,
          &ServerIpcService::OnErrorOccurred);
}

bool ServerIpcService::StartServer() {
  qDebug() << __FUNCTION__;
  if (server_) {
    return true;
  }
  server_ = new QLocalServer(this);
  if (!server_->listen("SharedlibNameServer")) {
    QMessageBox::critical(
        nullptr, tr("Local Fortune Server"),
        tr("Unable to start the server: %1.").arg(server_->errorString()));
    return false;
  }
  connect(server_, &QLocalServer::newConnection, this,
          &ServerIpcService::OnNewConnection);
  return true;
}

void ServerIpcService::OnNewConnection() {
#if 0
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_10);
  static quint64 connection_count = 0;
  QString message =
      QString("OnNewConnection Number: %1").arg(++connection_count);
  out << quint32(message.size());
  out << message;

  QLocalSocket* clientConnection = server_->nextPendingConnection();
  connect(clientConnection, &QLocalSocket::disconnected, clientConnection,
          &QLocalSocket::deleteLater);

  clientConnection->write(block);
  clientConnection->flush();
  clientConnection->disconnectFromServer();
#else
  socket_ = server_->nextPendingConnection();
  server_->close();  // because only listen one
  connect(socket_, &QLocalSocket::disconnected, this,
          &ServerIpcService::OnSocketDisconnected);
  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);
  connect(socket_, &QLocalSocket::readyRead, this,
          &ServerIpcService::OnReadyRead);
  connect(socket_, &QLocalSocket::errorOccurred, this,
          &ServerIpcService::OnErrorOccurred);
  WriteData("OnNewConnection");
#endif
}

void ServerIpcService::OnSocketDisconnected() {
  socket_->deleteLater();
  socket_ = nullptr;
  emit(ClientDisconnected());
}
void ServerIpcService::OnReadyRead() {
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
void ServerIpcService::OnErrorOccurred(
    QLocalSocket::LocalSocketError socketError) {}

void ServerIpcService::WriteData(const QString& data) {
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_10);
  QString message = data;
  out << quint32(message.size());
  out << message;
  socket_->write(block);
  socket_->flush();
}
