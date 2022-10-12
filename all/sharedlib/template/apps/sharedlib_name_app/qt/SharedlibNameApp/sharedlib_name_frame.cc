// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name_frame.h"

#include <QMessageBox>

#include "./ui_sharedlib_name_frame.h"

#include "sharedlib_name/sharedlib_name.h"

SharedlibNameFrame::SharedlibNameFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::SharedlibNameFrame) {
  ui_->setupUi(this);
  connect(ui_->actionPop, &QAction::triggered, this,
          &SharedlibNameFrame::PopWindow);
  connect(ui_->actionExit, &QAction::triggered, this,
          &SharedlibNameFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &SharedlibNameFrame::About);
}

SharedlibNameFrame::~SharedlibNameFrame() { delete ui_; }

void SharedlibNameFrame::PopWindow() {}

void SharedlibNameFrame::Exit() { QCoreApplication::quit(); }

void SharedlibNameFrame::About() {
  std::string version = SHAREDLIB_NAME::GetVersion();
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(
      this, tr("SharedlibNameApp"),
      tr("<b>SharedlibName</b> version ") + qstr_version);
}
