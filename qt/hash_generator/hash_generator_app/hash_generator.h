// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QObject>
#include <QThread>

class HashWorker : public QObject {
  Q_OBJECT

 public slots:
  void DoWork(const QString& file_path);

 signals:
  void HashReady(const QString& hash_code);
};

class HashGenerator : public QObject {
  Q_OBJECT

 public:
  HashGenerator();
  ~HashGenerator();

 public slots:
  void HandleHashCode(const QString&);

 signals:
  void Operate(const QString& file_path);

 private:
  QThread worker_;
};
