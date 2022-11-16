// Created by liangxu on 2022/11/16.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "transcoder_video_select_dialog.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>

#include "ui_transcoder_video_select_dialog.h"
#include "video_info_capture.h"


namespace {
QImage ConvertCvImageToQImage(const VideoInfoCapture::Image& cvimage,
                              double afactor = 1.0) {
  int cvwidth = cvimage.GetWidth();
  int cvheight = cvimage.GetHeight();
  QImage dest(cvwidth, cvheight, QImage::Format_ARGB32);
  for (int y = 0; y < cvheight; ++y) {
    for (int x = 0; x < cvwidth; ++x) {
      unsigned char r, g, b, a;
      cvimage.GetColor(x, y, &r, &g, &b, &a);
      a = a * afactor;
      QColor qcolor(r, g, b, a);
      dest.setPixelColor(x, y, qcolor);
    }
  }
  return dest;
}
}  // namespace

TranscoderVideoSelectDialog::TranscoderVideoSelectDialog(QWidget* parent)
    : QDialog(parent), ui_(new Ui::TranscoderVideoSelectDialog) {
  ui_->setupUi(this);
  connect(ui_->inputFilePath, &QLineEdit::textChanged, this,
          &TranscoderVideoSelectDialog::OnInputFileChanged);
  connect(ui_->inputFileButton, &QPushButton::clicked, this,
          &TranscoderVideoSelectDialog::OnInputFileBrowserClicked);

  auto file_formats = VideoInfoCapture::AvailableFileFormats();
  int index = 0;
  for (const auto& file_format : file_formats) {
    ui_->fileFormatCombo->addItem(file_format.c_str());
    if (file_format == "mp4") {
      ui_->fileFormatCombo->setCurrentIndex(index);
    }
    index++;
  }

  auto vencoders = VideoInfoCapture::AvailableVideoEncoders();
  index = 0;
  for (const auto& vencoder : vencoders) {
    ui_->videoEncoderCombo->addItem(vencoder.c_str());
    if (vencoder == "h264") {
      ui_->videoEncoderCombo->setCurrentIndex(index);
    }
    index++;
  }

  auto aencoders = VideoInfoCapture::AvailableAudioEncoders();
  index = 0;
  for (const auto& aencoders : aencoders) {
    ui_->audioEncoderCombo->addItem(aencoders.c_str());
    if (aencoders == "aac") {
      ui_->audioEncoderCombo->setCurrentIndex(index);
    }
    index++;
  }
}

TranscoderVideoSelectDialog::~TranscoderVideoSelectDialog() { delete ui_; }

void TranscoderVideoSelectDialog::SetLastInputFile(const QString& inputfile) {
  ui_->inputFilePath->setText(inputfile);
}
QString TranscoderVideoSelectDialog::GetLastInputFile() const {
  return ui_->inputFilePath->text();
}
QString TranscoderVideoSelectDialog::GetLastFileFormat() const {
  return ui_->fileFormatCombo->currentText();
}
QString TranscoderVideoSelectDialog::GetLastVideoEncoder() const {
  return ui_->videoEncoderCombo->currentText();
}
QString TranscoderVideoSelectDialog::GetLastAudioEncoder() const {
  return ui_->audioEncoderCombo->currentText();
}

int TranscoderVideoSelectDialog::GetSelectedWidth() const {
  return ui_->videoWidth->text().toInt();
}
int TranscoderVideoSelectDialog::GetSelectedHeight() const {
  return ui_->videoHeight->text().toInt();
}

double TranscoderVideoSelectDialog::GetVideoStartTime() const {
  return ui_->videoStartTime->text().toDouble();
}
double TranscoderVideoSelectDialog::GetVideoRecordTime() const {
  return ui_->videoRecordTime->text().toDouble();
}

QString TranscoderVideoSelectDialog::GetVideoBitrate() const {
  return ui_->videoBitrateCombo->currentText();
}
QString TranscoderVideoSelectDialog::GetAudioBitrate() const {
  return ui_->audioBitrateCombo->currentText();
}

void TranscoderVideoSelectDialog::OnInputFileChanged(const QString& inputfile) {
  QFileInfo file_info(inputfile);
  if (!file_info.exists() && file_info.isFile()) {
    return;
  }
  unsigned int duration;
  auto cvimage = VideoInfoCapture::ExtractVideoFirstValidFrameToImageBuffer(
      inputfile.toStdString().c_str(), &duration);
  if (!cvimage) {
    return;
  }

  QImage qimage = ConvertCvImageToQImage(*cvimage, 0.1);
  ui_->imageHolder->setPixmap(QPixmap::fromImage(qimage));
  ui_->imageHolder->setMaximumSize(800, 600);

  if (cvimage) {
    const uint32_t dura_s = duration;
    ui_->videoWidth->setValidator(
        new QIntValidator(0, cvimage->GetWidth(), this));
    ui_->videoHeight->setValidator(
        new QIntValidator(0, cvimage->GetHeight(), this));
    ui_->videoWidth->setText(QString("%1").arg(cvimage->GetWidth()));
    ui_->videoHeight->setText(QString("%1").arg(cvimage->GetHeight()));
    ui_->videoStartTime->setValidator(new QDoubleValidator(0, dura_s, 1, this));
    ui_->videoRecordTime->setValidator(
        new QDoubleValidator(0, dura_s, 1, this));
    ui_->videoStartTime->setText(QString("%1").arg(0));
    ui_->videoRecordTime->setText(QString("%1").arg(dura_s));
  } else {
    ui_->videoWidth->setReadOnly(true);
    ui_->videoHeight->setReadOnly(true);
  }

  auto video_info =
      VideoInfoCapture::ExtractFileInfo(inputfile.toStdString().c_str());
  ui_->videoBitrateCombo->addItem("");
  for (const auto& vbitrate : video_info.video_stream_bitrates) {
    QString vbitratestr = QString("%1K").arg(vbitrate.second / 1000);
    ui_->videoBitrateCombo->addItem(vbitratestr);
  }
  ui_->audioBitrateCombo->addItem("");
  for (const auto& vbitrate : video_info.audio_stream_bitrates) {
    QString vbitratestr = QString("%1K").arg(vbitrate.second / 1000);
    ui_->audioBitrateCombo->addItem(vbitratestr);
  }
}

void TranscoderVideoSelectDialog::OnInputFileBrowserClicked() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setViewMode(QFileDialog::Detail);

  QStringList fileNames;
  if (dialog.exec()) {
    fileNames = dialog.selectedFiles();
  }

  if (!fileNames.isEmpty()) {
    SetLastInputFile(fileNames[0]);
  }
}
