// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class HashGeneratorFrame;
}
QT_END_NAMESPACE

class HashGenerator;
class HashGeneratorFrame : public QMainWindow {
  Q_OBJECT

 public:
  explicit HashGeneratorFrame(QWidget *parent = nullptr);
  ~HashGeneratorFrame();

 private slots:
  void OnHash();
  void Exit();
  void About();

 private:
  Ui::HashGeneratorFrame *ui_;
  QScopedPointer<HashGenerator> hash_generator_;
};
