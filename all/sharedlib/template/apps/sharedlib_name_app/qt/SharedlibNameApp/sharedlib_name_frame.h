// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SharedlibNameFrame;
}
QT_END_NAMESPACE

class SharedlibNameFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit SharedlibNameFrame(QWidget *parent = nullptr);
  ~SharedlibNameFrame();

 private slots:
  void PopWindow();
  void Exit();
  void About();

 private:
  Ui::SharedlibNameFrame *ui_;
};
