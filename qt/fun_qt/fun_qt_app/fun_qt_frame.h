// Created by liangxu on 2023/01/16.
//
// Copyright (c) 2023 The FunQt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class FunQtFrame;
}
QT_END_NAMESPACE

class FunQtFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit FunQtFrame(QWidget *parent = nullptr);
  ~FunQtFrame();

 private slots:
  void OnCreate();
  void Exit();
  void About();

 private:
  Ui::FunQtFrame *ui_;
};
