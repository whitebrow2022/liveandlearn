// Created by liangxu on 2022/11/13.
//
// Copyright (c) 2022 The QdbusClient Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class QdbusClientFrame;
}
QT_END_NAMESPACE

class QdbusClientFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit QdbusClientFrame(QWidget *parent = nullptr);
  ~QdbusClientFrame();

 private slots:
  void OnCreate();
  void Exit();
  void About();

 private:
  Ui::QdbusClientFrame *ui_;
};
