// Created by liangxu on 2022/11/16.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDialog>

namespace Ui {
class TranscoderVideoSelectDialog;
}

class TranscoderVideoSelectDialog : public QDialog {
  Q_OBJECT

 public:
  explicit TranscoderVideoSelectDialog(QWidget* parent = nullptr);
  ~TranscoderVideoSelectDialog();

  void SetLastInputFile(const QString& inputfile);
  QString GetLastInputFile() const;
  QString GetLastFileFormat() const;
  QString GetLastVideoEncoder() const;
  QString GetLastAudioEncoder() const;
  int GetSelectedWidth() const;
  int GetSelectedHeight() const;
  double GetVideoStartTime() const;
  double GetVideoRecordTime() const;
  QString GetVideoBitrate() const;
  QString GetAudioBitrate() const;

 private slots:
  void OnInputFileChanged(const QString& inputfile);
  void OnInputFileBrowserClicked();

 private:
  Ui::TranscoderVideoSelectDialog* ui_{nullptr};
};
