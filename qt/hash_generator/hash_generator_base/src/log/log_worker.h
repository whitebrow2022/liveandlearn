// Created by liangxu on 2022/11/25.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <queue>
#include <string>
#include <thread>  // NOLINT

class LogWorker {
 public:
  ~LogWorker();

 public:
  void InitLog(const std::string& log_path);
  void Write(const std::string& log_msg);
  void UninitLog();

 private:
  static void Run(LogWorker*);
  void DoWork();

 private:
  std::unique_ptr<std::thread> log_thread_;
  std::string log_path_;
  std::atomic_bool exit_{false};
  std::queue<std::string> log_msg_quene_;
  std::condition_variable cond_var_;
  std::mutex mutex_;
};
