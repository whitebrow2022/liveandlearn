// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ffmpeg_wrapper_frame.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>

#include "ffmpeg_wrapper/ffmpeg_wrapper.h"
#include "ffmpeg_wrapper/video_converter.h"
#include "ffmpeg_wrapper/video_info_capture.h"
#include "ui_ffmpeg_wrapper_frame.h"
#include "video_converter_dialog.h"

namespace {
QImage ConvertCvImageToQImage(const VideoInfoCapture::Image& cvimage) {
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

FfmpegWrapperFrame::FfmpegWrapperFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::FfmpegWrapperFrame) {
  ui_->setupUi(this);
  connect(ui_->actionFirstFrame, &QAction::triggered, this,
          &FfmpegWrapperFrame::ExtractFirstValidFrame);
  connect(ui_->actionTestApi, &QAction::triggered, this,
          &FfmpegWrapperFrame::TestApi);
  connect(ui_->actionExit, &QAction::triggered, this,
          &FfmpegWrapperFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &FfmpegWrapperFrame::About);
  connect(ui_->actionToMp4, &QAction::triggered, this,
          &FfmpegWrapperFrame::ToMp4);
}

FfmpegWrapperFrame::~FfmpegWrapperFrame() { delete ui_; }

void FfmpegWrapperFrame::TestApi() { VideoToRawVideoAndRawAudio(); }

void FfmpegWrapperFrame::ExtractFirstValidFrame() {
  std::string video_path = GetVideoPath();
  QFileInfo file_info(QString(GetVideoPath().c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
  }
  unsigned int duration;
  auto cvimage = VideoInfoCapture::ExtractVideoFirstValidFrameToImageBuffer(
      video_path.c_str(), &duration);
  if (!cvimage) {
    return;
  }
  QImage qimage = ConvertCvImageToQImage(*cvimage);
  ui_->imageHolder->setPixmap(QPixmap::fromImage(qimage));
  ui_->videoDuration->setText(QString("时长: %1秒").arg(duration));
  ui_->imageHolder->setMaximumSize(800, 600);
}

void FfmpegWrapperFrame::Exit() { QCoreApplication::quit(); }

void FfmpegWrapperFrame::ToMp4() {
  if (converter_) {
    return;
  }

  std::string video_path = GetVideoPath();
  QFileInfo file_info(QString(GetVideoPath().c_str()));
  if (!file_info.exists() && file_info.isFile()) {
    QMessageBox::warning(this, "opencv",
                         QString("%1 not exists").arg(video_path.c_str()));
    return;
  }

  VideoConverterDialog converter_dlg(this);
  converter_dlg.SetLastInputFile(video_path.c_str());
  int ret = converter_dlg.exec();
  if (ret == 0) {
    return;
  }

  video_path = converter_dlg.GetLastInputFile().toStdString();
  if (video_path.empty()) {
    return;
  }

  QString frames_ouput_dir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  QDir dir(frames_ouput_dir);
  QString sub_dir_name = "mp4s";
  if (!dir.exists(sub_dir_name)) {
    dir.mkdir(sub_dir_name);
  }
  dir.cd(sub_dir_name);
  qDebug() << "output dir path is : " << dir.absolutePath();
  QString curr_time_str =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString mp4_file_path =
      QString("%1/example_%2.mp4").arg(dir.absolutePath()).arg(curr_time_str);

  VideoConverter::Request req;
  req.input_file = video_path;
  req.output_file = mp4_file_path.toStdString();
  req.video_encoder = converter_dlg.GetLastVideoEncoder().toStdString();
  req.audio_encoder = converter_dlg.GetLastAudioEncoder().toStdString();
  req.output_file_format = converter_dlg.GetLastFileFormat().toStdString();
  req.output_video_width = converter_dlg.GetSelectedWidth();
  req.output_video_start_time = converter_dlg.GetVideoStartTime();
  req.output_video_record_time = converter_dlg.GetVideoRecordTime();
  req.output_video_bitrate = converter_dlg.GetVideoBitrate().toStdString();
  req.output_audio_bitrate = converter_dlg.GetAudioBitrate().toStdString();

  Transcode(req);

  last_input_file_ = video_path.c_str();
}

void FfmpegWrapperFrame::About() {
  std::string version = FFMPEG_WRAPPER::GetVersion();
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("FfmpegWrapperApp"),
                     tr("<b>FfmpegWrapper</b> version ") + qstr_version);
}

void FfmpegWrapperFrame::VideoToRawVideoAndRawAudio() {
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
  QString mp3_file_path =
      QString("%1/example_%2.mp3").arg(dir.absolutePath()).arg(curr_time_str);
  if (FFMPEG_WRAPPER::ConvertVideoToRawVideoAndRawAudio(
          video_path.c_str(), mp4_file_path.toStdString().c_str(),
          mp3_file_path.toStdString().c_str())) {
    QFileInfo file_info(mp4_file_path);
    if (!file_info.exists() && file_info.isFile()) {
      QMessageBox::warning(this, "opencv",
                           QString("%1 not exists").arg(mp4_file_path));
      return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(mp4_file_path));
  }
}

void FfmpegWrapperFrame::OnConvertBegin(VideoConverter::Response r) {}
void FfmpegWrapperFrame::OnConvertProgress(VideoConverter::Response r) {
  static int count = 0;
  if (count > 3) {
    count = 0;
  }
  QString trail;
  for (int i = 0; i < count; i++) {
    trail.append(".");
  }
  ui_->converterStatus->setText(QString("正在转码%1").arg(trail));

  count++;
}
void FfmpegWrapperFrame::OnConvertEnd(VideoConverter::Response r) {
  // Play
  QMetaObject::invokeMethod(
      this,
      [r, this]() {
        converter_.reset();
#if 1
        QString mp4_file = r.output_file.c_str();
        QFileInfo file_info(mp4_file);
        if (!file_info.exists() && file_info.isFile()) {
          return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(mp4_file));
#endif

        ui_->converterStatus->setText(QString("转码完成"));
      },
      Qt::QueuedConnection);
}

void FfmpegWrapperFrame::Transcode(VideoConverter::Request req) {
  if (converter_) {
    return;
  }

#if 1
  converter_ = VideoConverter::MakeVideoConverter(
      req, this, [this](std::function<void()> work_fun) {
        QMetaObject::invokeMethod(
            this, [work_fun]() { work_fun(); }, Qt::QueuedConnection);
      });
#else
  converter_ = VideoConverter::MakeVideoConverter(req, this, nullptr);
#endif
  converter_->Start();
}

std::string FfmpegWrapperFrame::GetVideoPath() const {
  std::string video_path = "D:/osknief/imsdk/hevc_8k60P_bilibili_1.mp4";
  QFileInfo file_info(QString(video_path.c_str()));
  if (!(file_info.exists() && file_info.isFile())) {
#if defined(PROJECT_RESOURCES_DIR)
    video_path = PROJECT_RESOURCES_DIR;
    video_path.append("/example.mov");
#endif
    if (!last_input_file_.isEmpty()) {
      video_path = last_input_file_.toStdString();
    }
  }
  return video_path;
}
