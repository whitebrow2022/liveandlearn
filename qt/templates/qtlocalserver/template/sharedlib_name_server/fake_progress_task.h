// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QObject>

class FakeProgressTask : public QObject {
  Q_OBJECT
 public:
  explicit FakeProgressTask(QObject* parent = nullptr);

  void Start();
  void Stop();
  bool IsRunning() const { return task_id_ != 0; }

 signals:
  void OnTaskStarted();
  void OnTaskProgressed(double progress);
  void OnTaskStoped();

 protected:
  void timerEvent(QTimerEvent* event) override;

 private:
  int task_id_{0};
  quint32 curr_tick_count_{0};
};
