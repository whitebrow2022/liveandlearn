// Created by liangxu on 2022/12/18.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_video_info_dialog.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>

#include "transcoder_base/log/log_writer.h"
#include "ui_transcoder_video_info_dialog.h"
#include "video_info_capture.h"

TranscoderVideoInfoDialog::TranscoderVideoInfoDialog(QWidget* parent)
    : QDialog(parent), ui_(new Ui::TranscoderVideoInfoDialog) {
  ui_->setupUi(this);
  connect(ui_->inputFileButton, &QPushButton::clicked, this,
          &TranscoderVideoInfoDialog::OnInputFileBrowserClicked);
}

TranscoderVideoInfoDialog::~TranscoderVideoInfoDialog() { delete ui_; }

void TranscoderVideoInfoDialog::OnInputFileBrowserClicked() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setViewMode(QFileDialog::Detail);
  QString video_init_dir =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  dialog.setDirectory(video_init_dir);
  const QStringList filters({"Video files (*.mp4 *.mov *.mkv)",
                             "Mp4 files (*.mp4)", "Any files (*)"});
  dialog.setNameFilters(filters);

  QStringList file_names;
  if (dialog.exec()) {
    file_names = dialog.selectedFiles();
  }
  if (!file_names.isEmpty()) {
    QJsonArray json_arr;
    for (auto file_name : file_names) {
      VideoInfoCapture::VideoInfo video_info;
      QJsonObject json_obj;
      auto begin = std::chrono::high_resolution_clock::now();
      if (!VideoInfoCapture::ExtractVideInfo(file_name.toStdString().c_str(),
                                             &video_info)) {
        // error info
        json_obj.insert("err_code", -1);
        json_obj.insert("err_msg", QString("get video info failed."));
      } else {
        json_obj.insert("err_code", 0);
        json_obj.insert("err_msg", QString(""));
        QJsonObject json_thumb;
        json_thumb.insert(
            "image_path",
            QString("%1").arg(video_info.thumb_image_path.c_str()));
        json_thumb.insert("width", video_info.thumb_image_width);
        json_thumb.insert("height", video_info.thumb_image_height);
        json_thumb.insert("size", video_info.thumb_image_size);
        json_obj.insert("thumb_image", json_thumb);
        json_obj.insert("size", video_info.video_size);
        json_obj.insert("width", video_info.video_width);
        json_obj.insert("height", video_info.video_height);
        json_obj.insert("duration", video_info.video_duration);
        json_obj.insert("video_path",
                        QString("%1").arg(video_info.video_path.c_str()));
      }
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> elapsed = end - begin;
      json_obj.insert("extract_cost_time", static_cast<int>(elapsed.count()));
      json_arr.push_back(json_obj);
    }
    QJsonObject json_root;
    json_root.insert("video_info_list", json_arr);
    QJsonDocument doc(json_root);
    QString video_info_json_str(doc.toJson(QJsonDocument::Indented));
    ui_->outputEdit->setText(video_info_json_str);
  }
}
