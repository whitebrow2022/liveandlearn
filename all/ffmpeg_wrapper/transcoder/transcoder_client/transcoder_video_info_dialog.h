// Created by liangxu on 2022/12/18.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QDialog>

namespace Ui {
class TranscoderVideoInfoDialog;
}

class TranscoderVideoInfoDialog : public QDialog {
  Q_OBJECT

 public:
  explicit TranscoderVideoInfoDialog(QWidget* parent = nullptr);
  ~TranscoderVideoInfoDialog();

 private slots:
  void OnInputFileBrowserClicked();

 private:
  Ui::TranscoderVideoInfoDialog* ui_{nullptr};
};
