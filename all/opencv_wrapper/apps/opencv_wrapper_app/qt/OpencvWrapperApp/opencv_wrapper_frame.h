// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class OpencvWrapperFrame;
}
QT_END_NAMESPACE

class QMediaPlayer;
class QVideoWidget;
class OpencvWrapperFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit OpencvWrapperFrame(QWidget *parent = nullptr);
  ~OpencvWrapperFrame();

 private slots:
  void FirstImage();
  void ToImages();
  void ToMp4();
  void Exit();
  void About();

 private:
  std::string GetVideoPath() const;

  void InitMediaPlayer();
  void PlayVideo(const QString &video_path);

 private:
  Ui::OpencvWrapperFrame *ui_;

  QMediaPlayer *player_{nullptr};
  QVideoWidget *video_wgt_{nullptr};
};
