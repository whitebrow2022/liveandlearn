// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_server_frame.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "fake_progress_task.h"
#include "server_ipc_service.h"
#include "ui_transcoder_server_frame.h"

TranscoderServerFrame::TranscoderServerFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::TranscoderServerFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &TranscoderServerFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &TranscoderServerFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &TranscoderServerFrame::About);
}

TranscoderServerFrame::~TranscoderServerFrame() { delete ui_; }

void TranscoderServerFrame::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (!service_) {
    service_ = new ServerIpcService(this);
    connect(service_, &ServerIpcService::ClientDisconnected, this,
            &TranscoderServerFrame::OnClientDisconnected);
    connect(service_, &ServerIpcService::DataChanged, this,
            &TranscoderServerFrame::OnClientDataChanged);
  }
  service_->StartServer();
}

void TranscoderServerFrame::OnCreate() {
  QMessageBox::about(this, "TranscoderServer",
                     "Please <b>create</b> something.");
}

void TranscoderServerFrame::Exit() { QCoreApplication::quit(); }

void TranscoderServerFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("TranscoderServer"),
                     tr("<b>TranscoderServer</b> version ") + qstr_version);
}

void TranscoderServerFrame::OnClientDisconnected() {
  service_->deleteLater();
  service_ = nullptr;
  close();
}

void TranscoderServerFrame::OnClientDataChanged(const QString& data) {
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
              &TranscoderServerFrame::OnTaskStarted);
      connect(task_, &FakeProgressTask::OnTaskProgressed, this,
              &TranscoderServerFrame::OnTaskProgressed);
      connect(task_, &FakeProgressTask::OnTaskStoped, this,
              &TranscoderServerFrame::OnTaskStoped);
    }
    if (task_ && !task_->IsRunning()) {
      task_->Start();
    }
  }
}

void TranscoderServerFrame::OnTaskStarted() {
  QJsonObject json_obj;
  json_obj["state"] = "started";
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
void TranscoderServerFrame::OnTaskProgressed(double progress) {
  QJsonObject json_obj;
  json_obj["state"] = "progressed";
  json_obj["progress"] = progress;
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
void TranscoderServerFrame::OnTaskStoped() {
  QJsonObject json_obj;
  json_obj["state"] = "stoped";
  QJsonDocument doc(json_obj);
  QString json_str(doc.toJson(QJsonDocument::Compact));
  service_->Write(json_str);
}
