// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fake_progress_task.h"

constexpr const quint32 kMaxTimerTickCount = 100;

FakeProgressTask::FakeProgressTask(QObject* parent/* = nullptr*/)
    : QObject(parent) {}

void FakeProgressTask::Start() {
  curr_tick_count_ = 0;
  task_id_ = startTimer(100);
  emit(OnTaskStarted());
}

void FakeProgressTask::timerEvent(QTimerEvent* event) {
  curr_tick_count_++;
  double progress = static_cast<double>(curr_tick_count_) / kMaxTimerTickCount;
  emit(OnTaskProgressed(progress));

  if (curr_tick_count_ >= kMaxTimerTickCount) {
    emit(OnTaskStoped());
    killTimer(task_id_);
    task_id_ = 0;
  }
}
