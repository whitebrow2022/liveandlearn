// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>
#include <memory>

#include "ffmpeg_wrapper/video_converter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class FfmpegWrapperFrame;
}
QT_END_NAMESPACE

class FfmpegWrapperFrame : public QMainWindow, public VideoConverter::Delegate {
  Q_OBJECT

 public:
  explicit FfmpegWrapperFrame(QWidget* parent = nullptr);
  ~FfmpegWrapperFrame();

 private slots:
  void FirstFrame();
  void ExtractFirstValidFrame();
  void ToMp4();
  void Exit();
  void About();

 private:
  void VideoToRawVideoAndRawAudio();

 private:
  std::string GetVideoPath() const;

 private:
  void OnConvertBegin(VideoConverter::Response r) override;
  void OnConvertProgress(VideoConverter::Response r) override;
  void OnConvertEnd(VideoConverter::Response r) override;
  void Transcode(VideoConverter::Request r);

 private:
  Ui::FfmpegWrapperFrame* ui_;
  std::unique_ptr<VideoConverter> converter_;
  QString last_input_file_;
};
