// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name_client_frame.h"

#include <QMessageBox>
#include <QKeyEvent>

#include "ui_sharedlib_name_client_frame.h"

#include "server_driver.h"
#include "client_ipc_service.h"

SharedlibNameClientFrame::SharedlibNameClientFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::SharedlibNameClientFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &SharedlibNameClientFrame::OnCreate);
  connect(ui_->actionConnectToServer, &QAction::triggered, this,
          &SharedlibNameClientFrame::OnConnectToServer);
  connect(ui_->actionExit, &QAction::triggered, this,
          &SharedlibNameClientFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &SharedlibNameClientFrame::About);
  connect(ui_->sendButton, &QPushButton::clicked, this,
          &SharedlibNameClientFrame::OnClickedAndSend);
}

SharedlibNameClientFrame::~SharedlibNameClientFrame() { delete ui_; }

void SharedlibNameClientFrame::closeEvent(QCloseEvent* event) {
  if (driver_) {
    driver_->StopServer();
  }
}

void SharedlibNameClientFrame::OnCreate() {
  if (driver_ && driver_->IsServerRunning()) {
    return;
  }
  if (!driver_) {
    driver_ = new ServerDriver(this);
  }
  driver_->StartServer();
}

void SharedlibNameClientFrame::OnConnectToServer() {
  if (!(driver_ && driver_->IsServerRunning())) {
    return;
  }
  if (!client_) {
    client_ = new ClientIpcService(this);
    connect(client_, &ClientIpcService::DataChanged, this,
            &SharedlibNameClientFrame::OnServerDataChanged);
  }
  client_->ConnectToServer();
}

void SharedlibNameClientFrame::Exit() { QCoreApplication::quit(); }

void SharedlibNameClientFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("SharedlibNameClient"),
                     tr("<b>SharedlibNameClient</b> version ") + qstr_version);
}

void SharedlibNameClientFrame::OnClickedAndSend() {
  if (client_) {
    client_->Write(ui_->inputEdit->toPlainText());
  }
}

void SharedlibNameClientFrame::OnServerDataChanged(const QString& data) {
  ui_->outputEdit->setText(data);
}
