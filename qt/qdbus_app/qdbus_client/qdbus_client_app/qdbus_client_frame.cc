// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusClient Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qdbus_client_frame.h"

#include <QMessageBox>

#include "./ui_qdbus_client_frame.h"

QdbusClientFrame::QdbusClientFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::QdbusClientFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &QdbusClientFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &QdbusClientFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &QdbusClientFrame::About);
}

QdbusClientFrame::~QdbusClientFrame() { delete ui_; }

void QdbusClientFrame::OnCreate() {
  QMessageBox::about(this, "QdbusClientApp",
                     "Please <b>create</b> something.");
}

void QdbusClientFrame::Exit() { QCoreApplication::quit(); }

void QdbusClientFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("QdbusClientApp"),
                     tr("<b>QdbusClient</b> version ") + qstr_version);
}
