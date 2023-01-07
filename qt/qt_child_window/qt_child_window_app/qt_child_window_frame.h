// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class QtChildWindowFrame;
}
QT_END_NAMESPACE

class QtChildWindowFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit QtChildWindowFrame(QWidget *parent = nullptr);
  ~QtChildWindowFrame();

 private slots:
  void OnCreate();
  void Exit();
  void About();

 private:
  Ui::QtChildWindowFrame *ui_;
};
