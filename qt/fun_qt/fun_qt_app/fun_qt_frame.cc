// Created by liangxu on 2023/01/16.
//
// Copyright (c) 2023 The FunQt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fun_qt_frame.h"

#include <QMessageBox>

#include "./ui_fun_qt_frame.h"

FunQtFrame::FunQtFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::FunQtFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &FunQtFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &FunQtFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &FunQtFrame::About);
}

FunQtFrame::~FunQtFrame() { delete ui_; }

void FunQtFrame::OnCreate() {
  QMessageBox::about(this, "FunQtApp",
                     "Please <b>create</b> something.");
}

void FunQtFrame::Exit() { QCoreApplication::quit(); }

void FunQtFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("FunQtApp"),
                     tr("<b>FunQt</b> version ") + qstr_version);
}
