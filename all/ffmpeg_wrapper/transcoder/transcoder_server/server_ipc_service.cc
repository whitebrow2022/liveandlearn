// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "server_ipc_service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "server_ipc_service_c.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/log.h"
#ifdef __cplusplus
}
#endif

#if defined(_WIN32)
#include <Windows.h>
#endif

ServerIpcService& ServerIpcService::GetInstance() {
  static ServerIpcService inst;
  return inst;
}

ServerIpcService::ServerIpcService(QObject* parent /* = nullptr*/)
    : QObject(parent), socket_(new QLocalSocket(this)) {
  socket_stream_.setDevice(socket_);
  socket_stream_.setVersion(QDataStream::Qt_5_10);
  connect(socket_, &QLocalSocket::readyRead, this,
          &ServerIpcService::OnReadyRead);
  connect(socket_, &QLocalSocket::errorOccurred, this,
          &ServerIpcService::OnErrorOccurred);
  connect(socket_, &QLocalSocket::stateChanged, this,
          &ServerIpcService::OnSocketStateChanged);
  connect(socket_, &QLocalSocket::connected, this,
          &ServerIpcService::OnSocketConnected);
  connect(socket_, &QLocalSocket::disconnected, this,
          &ServerIpcService::OnSocketDisconnected);
}

void ServerIpcService::ConnectToServer(const QString& server_name) {
  if (!socket_) {
    return;
  }

  socket_->connectToServer(server_name);
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

void ServerIpcService::OnSocketStateChanged(
    QLocalSocket::LocalSocketState socket_state) {
  qInfo() << "socket_state: " << socket_state;
  socket_state_ = socket_state;
}

void ServerIpcService::OnSocketConnected() {
  // start transcoder
  emit(TranscoderReady());
}
void ServerIpcService::OnSocketDisconnected() {}

void ServerIpcService::WriteData(const QString& data) {
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_10);
  QString message = data;
  out << quint32(message.size());
  out << message;
  socket_->write(block);
  socket_->flush();
#if defined(WIN32)
  OutputDebugStringA("\n");
  OutputDebugStringA("WriteData: ");
  OutputDebugStringA(data.toUtf8().data());
  OutputDebugStringA("\n");
#endif
}

namespace {
void OutputLog(const char* str) {
#if defined(_WIN32)
  OutputDebugStringA(str);
#else
  printf("%s", str);
#endif
  QJsonObject json_obj;
  json_obj["type"] = 0;
  json_obj["log"] = QString(str);
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  ServerIpcService::GetInstance().Write(json_str);
}
}  // namespace

extern "C" {
void WriteToClient(const char* utf8_data) {
  QString qstr_data = utf8_data;
  ServerIpcService::GetInstance().Write(utf8_data);
}

void AvLog(void* avcl, int level, const char* fmt, ...) {
  if (!fmt) {
    OutputLog("format is nullptr");
    return;
  }

  // crash
#if 0
  // TODO(liangxu): fix vsnprintf crash.
  // mac vsnprintf crash, so use av_log
  {
    va_list arglist;
    va_start(arglist, fmt);
    av_log(avcl, level, fmt, arglist);
    va_end(arglist);
  }
#else
  if (AV_LOG_DEBUG == level) {
    return;
  }
  // Extra space for '\0'

  {
    va_list arglist;
    va_start(arglist, fmt);
#if defined(Q_OS_MACOS)
    int size_s = 4096;
#else
    int size_s = arglist ? (std::vsnprintf(nullptr, 0, fmt, arglist) + 1)
                         : strlen(fmt) + 1;
    if (size_s <= 0) {
      va_end(arglist);
      return;
    }
#endif
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(size);
    memset(buf.get(), 0, size);
    if (arglist) {
      std::vsnprintf(buf.get(), size, fmt, arglist);
    } else {
      std::snprintf(buf.get(), size, fmt);
    }
    va_end(arglist);
    // output to console
    OutputLog(buf.get());
  }
#endif
}

void GenerateAndPostProgressJson(double progress) {
  QJsonObject json_obj;
  json_obj["type"] = 1;
  json_obj["progress"] = progress;
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Indented));
  ServerIpcService::GetInstance().Write(json_str);
}
void PostStarted() { GenerateAndPostProgressJson(0.0); }
void PostProgress(double progress) { GenerateAndPostProgressJson(progress); }
void PostStoped() { GenerateAndPostProgressJson(100.0); }
}
