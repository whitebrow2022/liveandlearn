// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusServer Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qdbus_server_frame.h"

#include <QMessageBox>

#include "./ui_qdbus_server_frame.h"

QdbusServerFrame::QdbusServerFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::QdbusServerFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &QdbusServerFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &QdbusServerFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &QdbusServerFrame::About);
}

QdbusServerFrame::~QdbusServerFrame() { delete ui_; }

void QdbusServerFrame::OnCreate() {
  QMessageBox::about(this, "QdbusServerApp",
                     "Please <b>create</b> something.");
}

void QdbusServerFrame::Exit() { QCoreApplication::quit(); }

void QdbusServerFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("QdbusServerApp"),
                     tr("<b>QdbusServer</b> version ") + qstr_version);
}
