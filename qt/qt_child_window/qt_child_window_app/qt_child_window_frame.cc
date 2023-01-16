// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qt_child_window_frame.h"

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

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
  if (popped_) {
    popped_ = new QWidget();
    QVBoxLayout *main_layout = new QVBoxLayout(popped_);
    QPushButton *closeBtn = new QPushButton(popped_);
    closeBtn->setText("close");
    main_layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, popped_, &QWidget::close);
    popped_->setFixedSize({200, 100});
    popped_->setObjectName("popped");
    popped_->setStyleSheet(QString("QWidget#popped{ background-color: red; }"));

    popped_->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint |
                            Qt::NoDropShadowWindowHint);  // 无边框
  }
  QTimer::singleShot(5000, [this]() { popped_->show(); });
}

void QtChildWindowFrame::Exit() { QCoreApplication::quit(); }

void QtChildWindowFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("QtChildWindowApp"),
                     tr("<b>QtChildWindow</b> version ") + qstr_version);
}
