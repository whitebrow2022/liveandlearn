// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name_server_frame.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "fake_progress_task.h"
#include "server_ipc_service.h"
#include "ui_sharedlib_name_server_frame.h"

SharedlibNameServerFrame::SharedlibNameServerFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::SharedlibNameServerFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &SharedlibNameServerFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &SharedlibNameServerFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &SharedlibNameServerFrame::About);
}

SharedlibNameServerFrame::~SharedlibNameServerFrame() { delete ui_; }

void SharedlibNameServerFrame::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (!service_) {
    service_ = new ServerIpcService(this);
    connect(service_, &ServerIpcService::ClientDisconnected, this,
            &SharedlibNameServerFrame::OnClientDisconnected);
    connect(service_, &ServerIpcService::DataChanged, this,
            &SharedlibNameServerFrame::OnClientDataChanged);
  }
  service_->StartServer();
}

void SharedlibNameServerFrame::OnCreate() {
  QMessageBox::about(this, "SharedlibNameServer",
                     "Please <b>create</b> something.");
}

void SharedlibNameServerFrame::Exit() { QCoreApplication::quit(); }

void SharedlibNameServerFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("SharedlibNameServer"),
                     tr("<b>SharedlibNameServer</b> version ") + qstr_version);
}

void SharedlibNameServerFrame::OnClientDisconnected() {
  service_->deleteLater();
  service_ = nullptr;
  close();
}

void SharedlibNameServerFrame::OnClientDataChanged(const QString& data) {
  ui_->outputEdit->setText(data);

  // parse data
  QJsonDocument doc;
  QJsonParseError err;
  doc.fromJson(data.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError) {
    qDebug() << "parse err: " << err.errorString() << "; data: " << data;
    return;
  }
  QJsonObject json_obj = doc.object();
  bool run_long_task = true;
  if (json_obj.contains("run_long_task")) {
    run_long_task = json_obj.value("run_long_task").toBool(run_long_task);
  }
  if (run_long_task) {
    // start long time task
    if (!task_) {
      task_ = new FakeProgressTask(this);
      connect(task_, &FakeProgressTask::OnTaskStarted, this,
              &SharedlibNameServerFrame::OnTaskStarted);
      connect(task_, &FakeProgressTask::OnTaskProgressed, this,
              &SharedlibNameServerFrame::OnTaskProgressed);
      connect(task_, &FakeProgressTask::OnTaskStoped, this,
              &SharedlibNameServerFrame::OnTaskStoped);
    }
    if (task_ && !task_->IsRunning()) {
      task_->Start();
    }
  }
}

void SharedlibNameServerFrame::OnTaskStarted() {
  QJsonObject json_obj;
  json_obj["state"] = "started";
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
void SharedlibNameServerFrame::OnTaskProgressed(double progress) {
  QJsonObject json_obj;
  json_obj["state"] = "progressed";
  json_obj["progress"] = progress;
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
void SharedlibNameServerFrame::OnTaskStoped() {
  QJsonObject json_obj;
  json_obj["state"] = "stoped";
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
