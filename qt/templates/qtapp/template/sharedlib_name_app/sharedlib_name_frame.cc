// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name_frame.h"

#include <QMessageBox>

#include "./ui_sharedlib_name_frame.h"

SharedlibNameFrame::SharedlibNameFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::SharedlibNameFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &SharedlibNameFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &SharedlibNameFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &SharedlibNameFrame::About);
}

SharedlibNameFrame::~SharedlibNameFrame() { delete ui_; }

void SharedlibNameFrame::OnCreate() {
  QMessageBox::about(this, "SharedlibNameApp",
                     "Please <b>create</b> something.");
}

void SharedlibNameFrame::Exit() { QCoreApplication::quit(); }

void SharedlibNameFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("SharedlibNameApp"),
                     tr("<b>SharedlibName</b> version ") + qstr_version);
}
