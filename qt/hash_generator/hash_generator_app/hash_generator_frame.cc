// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hash_generator_frame.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "hash_generator.h"
#include "hash_generator_base/log/log_writer.h"
#include "ui_hash_generator_frame.h"

HashGeneratorFrame::HashGeneratorFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::HashGeneratorFrame) {
  ui_->setupUi(this);
  connect(ui_->actionHash, &QAction::triggered, this,
          &HashGeneratorFrame::OnHash);
  connect(ui_->actionExit, &QAction::triggered, this,
          &HashGeneratorFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &HashGeneratorFrame::About);
}

HashGeneratorFrame::~HashGeneratorFrame() { delete ui_; }

void HashGeneratorFrame::OnHash() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setViewMode(QFileDialog::Detail);

  QStringList fileNames;
  if (!dialog.exec()) {
    return;
  }
  fileNames = dialog.selectedFiles();
  if (fileNames.isEmpty()) {
    return;
  }

  QString selected_file = fileNames[0];

  // calc hash sha256
  if (!hash_generator_) {
    hash_generator_.reset(new HashGenerator());
  }
  hash_generator_->Operate(selected_file);
}

void HashGeneratorFrame::Exit() { QCoreApplication::quit(); }

void HashGeneratorFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("HashGeneratorApp"),
                     tr("<b>HashGenerator</b> version ") + qstr_version);
}
