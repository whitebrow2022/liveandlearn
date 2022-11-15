// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_client_frame.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>

#include "client_ipc_service.h"
#include "server_driver.h"
#include "ui_transcoder_client_frame.h"

TranscoderClientFrame::TranscoderClientFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::TranscoderClientFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &TranscoderClientFrame::OnCreate);
  connect(ui_->actionConnectToServer, &QAction::triggered, this,
          &TranscoderClientFrame::OnConnectToServer);
  connect(ui_->actionExit, &QAction::triggered, this,
          &TranscoderClientFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &TranscoderClientFrame::About);
  connect(ui_->sendButton, &QPushButton::clicked, this,
          &TranscoderClientFrame::OnClickedAndSend);

  ui_->inputEdit->setText(
      R"(
{
  "input": "D:/osknief/imsdk/example.mov",
  "output": "C:/Users/xiufeng.liu/Downloads/mp4s/tmppppp.mp4"
}
)");
}

TranscoderClientFrame::~TranscoderClientFrame() { delete ui_; }

void TranscoderClientFrame::closeEvent(QCloseEvent* event) {
  if (driver_) {
    driver_->StopServer();
  }
}

void TranscoderClientFrame::OnCreate() {
  QStringList ffmpeg_args;
  ffmpeg_args << "--help";
  StartServer(ffmpeg_args);
}

void TranscoderClientFrame::OnConnectToServer() {
  if (!(driver_ && driver_->IsServerRunning())) {
    return;
  }
  if (!client_) {
    client_ = new ClientIpcService(this);
    connect(client_, &ClientIpcService::DataChanged, this,
            &TranscoderClientFrame::OnServerDataChanged);
  }
  client_->StartServer();
}

void TranscoderClientFrame::Exit() { QCoreApplication::quit(); }

void TranscoderClientFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("TranscoderClient"),
                     tr("<b>TranscoderClient</b> version ") + qstr_version);
}

void TranscoderClientFrame::OnClickedAndSend() {
  if (!client_) {
    client_ = new ClientIpcService(this);
    connect(client_, &ClientIpcService::DataChanged, this,
            &TranscoderClientFrame::OnServerDataChanged);
  }
  client_->StartServer();

  QString ffmpeg_arg_str = ui_->inputEdit->toPlainText();
  // if (client_) {
  //   client_->Write(ffmpeg_arg_str);
  // }
  ui_->outputEdit->setText(ffmpeg_arg_str);
  ui_->outputEdit->append("\n");

  QJsonParseError err;
  auto doc = QJsonDocument::fromJson(ffmpeg_arg_str.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError) {
    qDebug() << "parse err: " << err.errorString()
             << "; data: " << ffmpeg_arg_str;
    return;
  }
  QJsonObject json_obj = doc.object();

  QString input_path;
  if (json_obj.contains("input")) {
    input_path = json_obj.value("input").toString("");
  }
  QString output_path;
  if (json_obj.contains("output")) {
    output_path = json_obj.value("output").toString("");
  }

  QFileInfo file_info(input_path);
  if (!(file_info.exists() && file_info.isFile())) {
    QMessageBox::warning(this, "transcoder",
                         QString("%1 not exists").arg(input_path));
    return;
  }

  QStringList arg_list;
  arg_list.append(client_->GetServerName());
  arg_list.append("-y");
  arg_list.append("-i");
  arg_list.append(input_path);
  arg_list.append("-c:v");
  arg_list.append("h264");
  arg_list.append("-c:a");
  arg_list.append("aac");
  arg_list.append(output_path);
  output_path_ = output_path;
  StartServer(arg_list);
}

void TranscoderClientFrame::OnServerDataChanged(const QString& data) {
  ui_->outputEdit->append(data);
}

void TranscoderClientFrame::StartServer(const QStringList& command_list) {
  if (driver_ && driver_->IsServerRunning()) {
    return;
  }
  if (!driver_) {
    driver_ = new ServerDriver(this);
    connect(driver_, &ServerDriver::ServerStarted, this, [this]() {
      // TODO: fix unconnected, client as server?
      //QTimer::singleShot(1000, this, &TranscoderClientFrame::OnConnectToServer);
    });
    connect(driver_, &ServerDriver::ServerFinished, this, [this](bool normal) {
      driver_->deleteLater();
      driver_ = nullptr;

      if (normal) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
#if 1
              QString mp4_file = output_path_;
              QFileInfo file_info(mp4_file);
              if (!(file_info.exists() && file_info.isFile())) {
                QMessageBox::warning(this, "transcoder",
                                     QString("%1 not exists").arg(mp4_file));
                ui_->outputEdit->append("transcode failed!");
                return;
              }
              QDesktopServices::openUrl(QUrl::fromLocalFile(mp4_file));
#endif
              ui_->outputEdit->append("transcode finished!");
            },
            Qt::QueuedConnection);
      } else {
        ui_->outputEdit->append("transcode failed!");
      }
    });
  }
  driver_->StartServer(command_list);
}
