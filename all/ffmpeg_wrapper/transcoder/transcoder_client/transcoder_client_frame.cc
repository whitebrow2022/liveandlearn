// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_client_frame.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>

#include "client_ipc_service.h"
#include "server_driver.h"
#include "transcoder_base/log/log_writer.h"
#include "transcoder_video_info_dialog.h"
#include "transcoder_video_select_dialog.h"
#include "ui_transcoder_client_frame.h"

constexpr const int kMaxProgressValue = 100;

TranscoderClientFrame::TranscoderClientFrame(QWidget* parent)
    : QMainWindow(parent), ui_(new Ui::TranscoderClientFrame) {
  ui_->setupUi(this);
  connect(ui_->actionTranscode, &QAction::triggered, this,
          &TranscoderClientFrame::OnTranscode);
  connect(ui_->actionVideoInfo, &QAction::triggered, this,
          &TranscoderClientFrame::OnVideoInfo);
  connect(ui_->actionExit, &QAction::triggered, this,
          &TranscoderClientFrame::Exit);
  connect(ui_->actionAbout, &QAction::triggered, this,
          &TranscoderClientFrame::About);
  connect(ui_->sendButton, &QPushButton::clicked, this,
          &TranscoderClientFrame::OnTranscode);

  ui_->inputEdit->setText(
      R"(
{
  "input": "D:/osknief/imsdk/example.mov",
  "output": "C:/Users/xiufeng.liu/Downloads/mp4s/tmppppp.mp4"
}
)");

  connect(this, &QObject::destroyed, this, [this]() { ui_ = nullptr; });

  ui_->transcodeProgressBar->setVisible(false);
  ui_->transcodeProgressBar->setRange(0, kMaxProgressValue);
  ui_->transcodeProgressBar->setValue(0);
}

TranscoderClientFrame::~TranscoderClientFrame() { delete ui_; }

void TranscoderClientFrame::closeEvent(QCloseEvent* event) {
  if (driver_) {
    driver_->StopServer();
  }
}

void TranscoderClientFrame::OnTranscode() {
  auto CheckVideoExists = [this](const QString& video_path) -> bool {
    QFileInfo file_info(video_path);
    if (!(file_info.exists() && file_info.isFile())) {
      QMessageBox::warning(this, "ffmpeg",
                           QString("%1 not exists").arg(video_path));
      return false;
    }
    return true;
  };
  auto video_path = GetSourceVideoPath();

  TranscoderVideoSelectDialog transcode_dlg(this);
  transcode_dlg.SetLastInputFile(video_path);
  if (transcode_dlg.exec() == 0) {
    return;
  }

  video_path = transcode_dlg.GetLastInputFile();
  if (video_path.isEmpty()) {
    return;
  }
  if (!CheckVideoExists(video_path)) {
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

  // convert to json
  QJsonObject json_obj;
  json_obj.insert("input", video_path);
  json_obj.insert("output", mp4_file_path);
  json_obj.insert("output_video_encoder", transcode_dlg.GetLastVideoEncoder());
  json_obj.insert("output_audio_encoder", transcode_dlg.GetLastAudioEncoder());
  json_obj.insert("output_format", transcode_dlg.GetLastFileFormat());
  json_obj.insert("output_width", transcode_dlg.GetSelectedWidth());
  json_obj.insert("output_height", transcode_dlg.GetSelectedHeight());
  json_obj.insert("output_start_time", transcode_dlg.GetVideoStartTime());
  json_obj.insert("output_record_time", transcode_dlg.GetVideoRecordTime());
  json_obj.insert("output_video_bitrate", transcode_dlg.GetVideoBitrate());
  json_obj.insert("output_audio_bitrate", transcode_dlg.GetAudioBitrate());
  QJsonDocument doc(json_obj);
  QString ffmpeg_param(doc.toJson(QJsonDocument::Indented));
  ui_->inputEdit->setText(ffmpeg_param);

  OnClickedAndSend();
}

void TranscoderClientFrame::OnVideoInfo() {
  TranscoderVideoInfoDialog video_info_dlg(this);
  if (video_info_dlg.exec() == 0) {
    return;
  }
}

void TranscoderClientFrame::Exit() { QCoreApplication::quit(); }

void TranscoderClientFrame::About() {
  std::string version = "1.0.0.0";
  QString qstr_version = QString::fromUtf8(version.c_str(), version.length());
  QMessageBox::about(this, tr("TranscoderClient"),
                     tr("<b>TranscoderClient</b> version ") + qstr_version);
}

void TranscoderClientFrame::OnClickedAndSend() {
  if (!client_) {
    client_ = new ClientIpcService(this);
    connect(client_, &ClientIpcService::DataChanged, this,
            &TranscoderClientFrame::OnServerDataChanged);
  }
  client_->StartServer();

  QString ffmpeg_arg_str = ui_->inputEdit->toPlainText();
  ui_->outputEdit->setText(ffmpeg_arg_str);
  ui_->outputEdit->append("\n");

  QJsonParseError err;
  auto doc = QJsonDocument::fromJson(ffmpeg_arg_str.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError) {
    qDebug() << "parse err: " << err.errorString()
             << "; data: " << ffmpeg_arg_str;
    return;
  }
  QJsonObject json_obj = doc.object();

  QString input_path;
  if (json_obj.contains("input")) {
    input_path = json_obj.value("input").toString("");
  }
  QString output_path;
  if (json_obj.contains("output")) {
    output_path = json_obj.value("output").toString("");
  }

  QFileInfo file_info(input_path);
  if (!(file_info.exists() && file_info.isFile())) {
    QMessageBox::warning(this, "transcoder",
                         QString("%1 not exists").arg(input_path));
    return;
  }

  QStringList arg_list;
  arg_list.append(client_->GetServerName());
  // global parameter
  arg_list.append("-y");
  // input parameter
  arg_list.append("-i");
  arg_list.append(input_path);
  last_input_file_ = input_path;
  // output parameter
  if (json_obj.contains("output_video_encoder")) {
    arg_list.append("-c:v");
    arg_list.append(json_obj.value("output_video_encoder").toString("h264"));
  }
  if (json_obj.contains("output_audio_encoder")) {
    arg_list.append("-c:a");
    arg_list.append(json_obj.value("output_audio_encoder").toString("aac"));
  }
  if (json_obj.contains("output_format")) {
    arg_list.append("-f");
    arg_list.append(json_obj.value("output_format").toString("mp4"));
  }
  if (json_obj.contains("output_start_time") &&
      json_obj.value("output_start_time").toDouble(0) != 0) {
    arg_list.append("-ss");
    arg_list.append(
        QString("%1").arg(json_obj.value("output_start_time").toDouble(0)));
  }
  if (json_obj.contains("output_record_time") &&
      json_obj.value("output_record_time").toDouble(0) != 0) {
    arg_list.append("-t");
    arg_list.append(
        QString("%1").arg(json_obj.value("output_record_time").toDouble(0)));
  }
  if (json_obj.contains("output_stop_time") &&
      json_obj.value("output_stop_time").toDouble(0) != 0) {
    arg_list.append("-to");
    arg_list.append(
        QString("%1").arg(json_obj.value("output_stop_time").toDouble(0)));
  }
  int output_video_width = -1;
  if (json_obj.contains("output_width")) {
    output_video_width = json_obj.value("output_width").toInt(-1);
  }
  int output_video_height = -1;
  if (json_obj.contains("output_height")) {
    output_video_height = json_obj.value("output_height").toInt(-1);
  }
  if (!(output_video_width == -1 && output_video_height == -1)) {
    arg_list.append("-filter:v");
    arg_list.append(QString("scale=%1:%2")
                        .arg(output_video_width)
                        .arg(output_video_height));
  }
  if (json_obj.contains("output_video_bitrate") &&
      !json_obj.value("output_video_bitrate").toString("").isEmpty()) {
    arg_list.append("-b:v");
    arg_list.append(json_obj.value("output_video_bitrate").toString("128K"));
  }
  if (json_obj.contains("output_audio_bitrate") &&
      !json_obj.value("output_audio_bitrate").toString("").isEmpty()) {
    arg_list.append("-b:a");
    arg_list.append(json_obj.value("output_audio_bitrate").toString("128K"));
  }
  arg_list.append(output_path);
  output_path_ = output_path;
  StartServer(arg_list);

  ui_->actionTranscode->setEnabled(false);
  ui_->sendButton->setEnabled(false);
  ui_->transcodeProgressBar->setVisible(true);
  ui_->transcodeProgressBar->setValue(0);
}

void TranscoderClientFrame::OnServerDataChanged(const QString& data) {
  ui_->outputEdit->append(data);

  QJsonParseError err;
  auto doc = QJsonDocument::fromJson(data.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError) {
    qDebug() << __FUNCTION__ << "parse err: " << err.errorString()
             << "; data: " << data;
    return;
  }
  QJsonObject json_obj = doc.object();
  if (json_obj.contains("type")) {
    auto type = json_obj.value("type").toInt(0);
    if (type == 1) {
      if (json_obj.contains("progress")) {
        auto progess = json_obj.value("progress").toDouble(0.0);
        ui_->transcodeProgressBar->setValue(progess * kMaxProgressValue);
      }
    }
  }
}

void TranscoderClientFrame::StartServer(const QStringList& command_list) {
  if (driver_ && driver_->IsServerRunning()) {
    return;
  }
  if (!driver_) {
    driver_ = new ServerDriver(this);
    connect(driver_, &ServerDriver::ServerStarted, this, [this]() {
      log_info << "transcoder started!";
      transcode_time_start_ = std::chrono::high_resolution_clock::now();
    });
    connect(driver_, &ServerDriver::ServerFinished, this, [this](bool normal) {
      auto time_end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> elapsed =
          time_end - transcode_time_start_;
      auto spend_secs = static_cast<uint32_t>(elapsed.count()) / 1000;
      log_info << "transcoder transcode " << last_input_file_.toStdString()
               << " to " << output_path_.toStdString()
               << " , spend time: " << spend_secs << " sec";
      log_info << "transcoder finished!";
      driver_->deleteLater();
      driver_ = nullptr;

      if (!ui_) {
        return;
      }

      ui_->actionTranscode->setEnabled(true);
      ui_->sendButton->setEnabled(true);
      ui_->transcodeProgressBar->setVisible(false);

      if (normal) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
#if 1
              QString mp4_file = output_path_;
              QFileInfo file_info(mp4_file);
              if (!(file_info.exists() && file_info.isFile())) {
                QMessageBox::warning(this, "transcoder",
                                     QString("%1 not exists").arg(mp4_file));
                ui_->outputEdit->append("transcode failed!");
                return;
              }
              QDesktopServices::openUrl(QUrl::fromLocalFile(mp4_file));
#endif
              ui_->outputEdit->append("transcode finished!");
            },
            Qt::QueuedConnection);
      } else {
        ui_->outputEdit->append("transcode failed!");
      }

      QMetaObject::invokeMethod(
          this,
          [this, spend_secs]() {
            QString spend_time_str =
                QString(
                    "transcoder transcode %1 to %2, spend time: %3 seconds!")
                    .arg(last_input_file_)
                    .arg(output_path_)
                    .arg(spend_secs);
            ui_->outputEdit->append("\n\n");
            ui_->outputEdit->append(spend_time_str);
          },
          Qt::QueuedConnection);
    });
  }
  driver_->StartServer(command_list);
}

QString TranscoderClientFrame::GetSourceVideoPath() const {
  QString video_path;
  if (!last_input_file_.isEmpty()) {
    video_path = last_input_file_;
  }
  return video_path;
}
