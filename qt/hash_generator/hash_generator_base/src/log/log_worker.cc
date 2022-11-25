// Created by liangxu on 2022/11/25.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "log_worker.h"

#include "hash_generator_base/log/log_writer.h"

LogWorker::~LogWorker() { UninitLog(); }

void LogWorker::InitLog(const std::string& log_path) {
  if (log_thread_) {
    return;
  }
  exit_ = false;
  log_path_ = log_path;
  log_thread_ = std::make_unique<std::thread>(std::thread(&Run, this));

  log_info << "";
}

void LogWorker::Write(const std::string& log_msg) {
  if (!log_thread_) {
    return;
  }
  {
    std::lock_guard<std::mutex> l{mutex_};
    log_msg_quene_.push(log_msg);
  }
  cond_var_.notify_one();
}
void LogWorker::UninitLog() {
  if (!log_thread_) {
    return;
  }
  log_info << "";
  exit_ = true;
  log_thread_->join();
  log_thread_.reset();
}
void LogWorker::Run(LogWorker* worker) { worker->DoWork(); }

void LogWorker::DoWork() {
  FILE* log_file{nullptr};
  log_file = fopen(log_path_.c_str(), "w");
  if (!log_file) {
    return;
  }

  // process log msg
  while (true) {
    if (exit_) {
      std::lock_guard<std::mutex> l{mutex_};
      while (!log_msg_quene_.empty()) {
        auto log_msg = log_msg_quene_.front();
        log_msg_quene_.pop();
        // write
        fwrite(log_msg.c_str(), 1, log_msg.length(), log_file);
        fwrite("\n", 1, 1, log_file);
      }
      break;
    }

    std::string log_msg;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      cond_var_.wait(lock, [this]() { return !log_msg_quene_.empty(); });
      // working
      log_msg = log_msg_quene_.front();
      log_msg_quene_.pop();
    }
    // write
    fwrite(log_msg.c_str(), 1, log_msg.length(), log_file);
    fwrite("\n", 1, 1, log_file);
  }

  fclose(log_file);
}
