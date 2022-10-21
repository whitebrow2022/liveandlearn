// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper_frame.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "./ui_opencv_wrapper_frame.h"
#include "opencv_wrapper/opencv_wrapper.h"

OpencvWrapperFrame::OpencvWrapperFrame(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::OpencvWrapperFrame) {
  ui_->setupUi(this);
  connect(ui_->actionPop, &QAction::triggered, this,
          &OpencvWrapperFrame::PopWindow);
  connect(ui_->actionExit, &QAction::triggered, this,
          &OpencvWrapperFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &OpencvWrapperFrame::About);
}

OpencvWrapperFrame::~OpencvWrapperFrame() { delete ui_; }

void OpencvWrapperFrame::PopWindow() {}

void OpencvWrapperFrame::Exit() { QCoreApplication::quit(); }

void OpencvWrapperFrame::About() {
#if 0
  std::string version = OPENCV_WRAPPER::GetVersion();
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(
      this, tr("OpencvWrapperApp"),
      tr("<b>OpencvWrapper</b> version ") + qstr_version);
#endif
  qDebug() << "QStandardPaths::DownloadLocation: "
           << QStandardPaths::writableLocation(
                  QStandardPaths::DownloadLocation);

  QString frames_ouput_dir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QDir dir(frames_ouput_dir);
  QString sub_dir_name = "opencvframes";
  if (!dir.exists(sub_dir_name)) {
    dir.mkdir(sub_dir_name);
  }
  dir.cd(sub_dir_name);
  qDebug() << "output dir path is : " << dir.absolutePath();
  std::string video_path = "/Users/xiufeng.liu/Downloads/example.mov";
#if defined(PROJECT_RESOURCES_DIR)
  video_path = PROJECT_RESOURCES_DIR;
#endif
  video_path.append("/example.mov");
  QFileInfo file_info(QString(video_path.c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
  }
  OPENCV_WRAPPER::ExtractVideoToFrames(
      video_path.c_str(), dir.absolutePath().toStdString().c_str());
}
