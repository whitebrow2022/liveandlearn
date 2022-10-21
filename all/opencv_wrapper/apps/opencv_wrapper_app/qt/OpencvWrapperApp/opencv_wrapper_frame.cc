// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper_frame.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QVideoWidget>

#include "./ui_opencv_wrapper_frame.h"
#include "opencv_wrapper/opencv_wrapper.h"

namespace {
QImage ConvertCvImageToQImage(const OPENCV_WRAPPER::CvImage& cvimage) {
  int cvwidth = cvimage.GetWidth();
  int cvheight = cvimage.GetHeight();
  QImage dest(cvwidth, cvheight, QImage::Format_ARGB32);
  for (int y = 0; y < cvheight; ++y) {
    for (int x = 0; x < cvwidth; ++x) {
      unsigned char r, g, b, a;
      cvimage.GetColor(x, y, &r, &g, &b, &a);
      QColor qcolor(r, g, b, a);
      dest.setPixelColor(x, y, qcolor);
    }
  }
  return dest;
}
}  // namespace

OpencvWrapperFrame::OpencvWrapperFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::OpencvWrapperFrame) {
  ui_->setupUi(this);
  connect(ui_->actionFirstImage, &QAction::triggered, this,
          &OpencvWrapperFrame::FirstImage);
  connect(ui_->actionToImages, &QAction::triggered, this,
          &OpencvWrapperFrame::ToImages);
  connect(ui_->actionToMp4, &QAction::triggered, this,
          &OpencvWrapperFrame::ToMp4);
  connect(ui_->actionExit, &QAction::triggered, this,
          &OpencvWrapperFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &OpencvWrapperFrame::About);

  InitMediaPlayer();
}

OpencvWrapperFrame::~OpencvWrapperFrame() { delete ui_; }

void OpencvWrapperFrame::FirstImage() {
  std::string video_path = GetVideoPath();
  QFileInfo file_info(QString(GetVideoPath().c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
  }
  unsigned int duration = 0;
  auto cvimage = OPENCV_WRAPPER::ExtractVideoFirstValidFrameToImageBuffer(
      video_path.c_str(), &duration);
  // cvimage to QImage
  if (!cvimage) {
    return;
  }
  QImage qimage = ConvertCvImageToQImage(*cvimage);
  ui_->imageHolder->setPixmap(QPixmap::fromImage(qimage));
  ui_->videoDuration->setText(QString("时长: %1秒").arg(duration));
}

void OpencvWrapperFrame::ToImages() {
  QString frames_ouput_dir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QDir dir(frames_ouput_dir);
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString sub_dir_name = "opencvframes_" + curr_time_str;
  if (!dir.exists(sub_dir_name)) {
    dir.mkdir(sub_dir_name);
  }
  dir.cd(sub_dir_name);
  qDebug() << "output dir path is : " << dir.absolutePath();
  std::string video_path = GetVideoPath();
  QFileInfo file_info(QString(GetVideoPath().c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
  }
  OPENCV_WRAPPER::ExtractVideoToFrames(
      video_path.c_str(), dir.absolutePath().toStdString().c_str());
  // QString image_file_path =
  //    QString("%1/example_first_valid_frame.png").arg(dir.absolutePath());
  // OPENCV_WRAPPER::ExtractVideoFirstValidFrameToImage(
  //    video_path.c_str(), image_file_path.toStdString().c_str());
}

void OpencvWrapperFrame::ToMp4() {
  QString frames_ouput_dir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QDir dir(frames_ouput_dir);
  QString sub_dir_name = "mp4s";
  if (!dir.exists(sub_dir_name)) {
    dir.mkdir(sub_dir_name);
  }
  dir.cd(sub_dir_name);
  qDebug() << "output dir path is : " << dir.absolutePath();
  std::string video_path = GetVideoPath();
  QFileInfo file_info(QString(GetVideoPath().c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
  }
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString mp4_file_path =
      QString("%1/example_%2.mp4").arg(dir.absolutePath()).arg(curr_time_str);
  if (OPENCV_WRAPPER::ConvertVideoToMp4(video_path.c_str(),
                                        mp4_file_path.toStdString().c_str())) {
    PlayVideo(mp4_file_path);
    // PlayVideo(QString("%1/test2.mp4").arg(frames_ouput_dir));
  }
}

void OpencvWrapperFrame::Exit() { QCoreApplication::quit(); }

void OpencvWrapperFrame::About() {
  std::string version = OPENCV_WRAPPER::GetVersion();
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("OpencvWrapperApp"),
                     tr("<b>OpencvWrapper</b> version ") + qstr_version);
}

std::string OpencvWrapperFrame::GetVideoPath() const {
  std::string video_path = "/Users/xiufeng.liu/Downloads/example.mov";
#if defined(PROJECT_RESOURCES_DIR)
  video_path = PROJECT_RESOURCES_DIR;
#endif
  video_path.append("/example.mov");
  return video_path;
}

void OpencvWrapperFrame::InitMediaPlayer() {
  if (player_ || video_wgt_) {
    return;
  }
  player_ = new QMediaPlayer(this);
  video_wgt_ = new QVideoWidget();
  player_->setVideoOutput(video_wgt_);

  connect(player_, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
          this, [this](QMediaPlayer::Error error) {
            qDebug() << "MediaPlayer Error: " << error << " "
                     << player_->errorString();
          });
}
void OpencvWrapperFrame::PlayVideo(const QString& video_path) {
  QFileInfo file_info(video_path);
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path));
    return;
  }
  QDesktopServices::openUrl(QUrl::fromLocalFile(video_path));
#if 0
  if (!player_) {
    return;
  }

  player_->setMedia(QUrl::fromLocalFile(video_path));
  video_wgt_->setGeometry(100, 100, 800, 600);
  video_wgt_->show();
  player_->play();

  qDebug() << "player state : " << player_->state();
#endif
}
