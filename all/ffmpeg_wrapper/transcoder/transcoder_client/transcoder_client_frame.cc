// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_client_frame.h"

#include <QMessageBox>
#include <QKeyEvent>

#include "ui_transcoder_client_frame.h"

#include "server_driver.h"
#include "client_ipc_service.h"

TranscoderClientFrame::TranscoderClientFrame(QWidget *parent)
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
}

TranscoderClientFrame::~TranscoderClientFrame() { delete ui_; }

void TranscoderClientFrame::closeEvent(QCloseEvent* event) {
  if (driver_) {
    driver_->StopServer();
  }
}

void TranscoderClientFrame::OnCreate() {
  if (driver_ && driver_->IsServerRunning()) {
    return;
  }
  if (!driver_) {
    driver_ = new ServerDriver(this);
  }
  driver_->StartServer();
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
  client_->ConnectToServer();
}

void TranscoderClientFrame::Exit() { QCoreApplication::quit(); }

void TranscoderClientFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("TranscoderClient"),
                     tr("<b>TranscoderClient</b> version ") + qstr_version);
}

void TranscoderClientFrame::OnClickedAndSend() {
  if (client_) {
    client_->Write(ui_->inputEdit->toPlainText());
  }
}

void TranscoderClientFrame::OnServerDataChanged(const QString& data) {
  ui_->outputEdit->setText(data);
}
