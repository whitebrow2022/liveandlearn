// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qt_child_window_frame.h"

#include <QMessageBox>

#include "qt_window_util.h"
#include "ui_qt_child_window_frame.h"

QtChildWindowFrame::QtChildWindowFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::QtChildWindowFrame) {
  ui_->setupUi(this);
  connect(ui_->actionCreate, &QAction::triggered, this,
          &QtChildWindowFrame::OnCreate);
  connect(ui_->actionExit, &QAction::triggered, this,
          &QtChildWindowFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &QtChildWindowFrame::About);
}

QtChildWindowFrame::~QtChildWindowFrame() { delete ui_; }

void QtChildWindowFrame::OnCreate() {
  QMessageBox msgBox;
#if defined(Q_OS_MACOS)
  msgBox.setText("Message box about swiping up with three fingers");
  msgBox.setInformativeText(
      "Solve the problem that the parent and child windows are separated by "
      "sliding up with three fingers on the mac system");
#else
  msgBox.setText("QMessageBox");
  msgBox.setInformativeText("More information");
#endif
  msgBox.setStandardButtons(QMessageBox::Ok);
  QtWindowUtil::AttachChildToParent(this, &msgBox);
  int ret = msgBox.exec();
  QtWindowUtil::DetachChildFromParent(this, &msgBox);
}

void QtChildWindowFrame::Exit() { QCoreApplication::quit(); }

void QtChildWindowFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("QtChildWindowApp"),
                     tr("<b>QtChildWindow</b> version ") + qstr_version);
}
