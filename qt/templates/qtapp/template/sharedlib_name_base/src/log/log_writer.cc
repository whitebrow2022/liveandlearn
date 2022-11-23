// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name_base/log/log_writer.h"

#if defined(OS_WINDOWS)
#include <Windows.h>
#include <debugapi.h>
#elif defined(OS_ANDROID)
#include <android/log.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(OS_MACOS)
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#endif

#include <chrono>  // NOLINT
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
std::string GetCurrentTimeStr() {
  auto curr_clock = std::chrono::system_clock::now();
  std::time_t curr_time = std::chrono::system_clock::to_time_t(curr_clock);
  auto curr_tm = std::localtime(&curr_time);
  auto curr_year = curr_tm->tm_year;
  auto curr_mon = curr_tm->tm_mon;
  auto curr_day = curr_tm->tm_mday;
  auto curr_hour = curr_tm->tm_hour;
  auto curr_min = curr_tm->tm_min;
  auto curr_sec = curr_tm->tm_sec;
  auto msec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                              curr_clock.time_since_epoch())
                              .count();
  auto floor_msec_since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(
          curr_clock.time_since_epoch())
          .count() *
      1000;
  auto curr_msec = msec_since_epoch - floor_msec_since_epoch;
  // auto curr_time_str = std::ctime(&curr_time);
  std::stringstream oss;
  oss << curr_year + 1900;
  oss << "/" << std::setw(2) << std::setfill('0') << curr_mon + 1;
  oss << "/" << std::setw(2) << std::setfill('0') << curr_day << " ";
  oss << std::setw(2) << std::setfill('0') << curr_hour;
  oss << ":" << std::setw(2) << std::setfill('0') << curr_min;
  oss << ":" << std::setw(2) << std::setfill('0') << curr_sec;
  oss << "." << std::setw(3) << std::setfill('0') << curr_msec;
  return oss.str();
}
constexpr const int kMaxFileNameCodeLineLen = 30;
std::string GenerateNewStrFromFileNameAndCodeLine(const char* full_file_name,
                                                  int32_t code_line) {
  std::string fncl_str;
  std::ostringstream oss;
  std::string file_name_str = full_file_name;
  auto last_backslash_pos = file_name_str.rfind('\\');
  auto last_slash_pos = file_name_str.rfind('/');
  size_t pos = std::string::npos;
  if (last_backslash_pos != std::string::npos) {
    pos = last_backslash_pos;
  }
  if (last_slash_pos != std::string::npos) {
    if (pos == std::string::npos) {
      pos = last_slash_pos;
    } else if (pos < last_slash_pos) {
      pos = last_slash_pos;
    }
  }
  file_name_str = file_name_str.substr(pos + 1);
  oss << file_name_str << ":" << code_line;
  fncl_str = oss.str();
  if (fncl_str.length() > kMaxFileNameCodeLineLen) {
    fncl_str = fncl_str.replace(
        3, (fncl_str.length() - (kMaxFileNameCodeLineLen - 3)), "...");
  }
  return fncl_str;
}

std::string GenerateOutputLog(int32_t log_level, const char* file_name,
                              int32_t code_line, const char* func_name,
                              const std::string& content) {
#if defined(OS_WINDOWS)
  DWORD process_id = ::GetCurrentProcessId();
  DWORD thread_id = ::GetCurrentThreadId();
#elif defined(OS_ANDROID)
  auto process_id = getpid();
  auto thread_id = gettid();
#elif defined(OS_MACOS)
  auto process_id = getpid();
  auto thread_id = pthread_self();
#endif
  auto file_name_code_line =
      GenerateNewStrFromFileNameAndCodeLine(file_name, code_line);
  auto curr_time_str = GetCurrentTimeStr();
  std::ostringstream oss;
  oss << std::left;
  auto old_flags = oss.flags();
  oss << curr_time_str;
  oss << "|" << std::setw(8) << process_id;
  oss << "|" << std::setw(8) << thread_id;
  oss << "|" << std::setw(kMaxFileNameCodeLineLen) << file_name_code_line;
  oss.flags(old_flags);
  oss << "|" << func_name;
  oss << "|" << content;
  return oss.str();
}
void PlatformOutputLog(const std::string& log_msg) {
#if defined(OS_WINDOWS)
  OutputDebugStringA(log_msg.c_str());
  if (IsDebuggerPresent()) {
    OutputDebugStringA("\n");
  }
#elif defined(OS_ANDROID)
  android_LogPriority priority = ANDROID_LOG_INFO;
  switch (log_level) {
    case log_level_debug:
      priority = ANDROID_LOG_DEBUG;
      break;
    case log_level_warning:
      priority = ANDROID_LOG_WARN;
      break;
    case log_level_cp:
    case log_level_info:
      priority = ANDROID_LOG_INFO;
      break;
    case log_level_cpe:
    case log_level_error:
      priority = ANDROID_LOG_ERROR;
      break;
    case log_level_fault:
      priority = ANDROID_LOG_FATAL;
      break;
    default:
      break;
  }
  __android_log_print(priority, "sharedlib_name", "%s", log_msg.c_str());
#elif defined(OS_MACOS)
  std::cout << log_msg << std::endl;
#endif
}
}  // namespace

BEGIN_NAMESPACE_SHAREDLIB_NAME_BASE

SHAREDLIB_NAME_BASE_API int32_t GetLogLevel() { return log_level_debug; }
SHAREDLIB_NAME_BASE_API void OutputLog(int32_t log_level, const char* file_name,
                                       int32_t code_line, const char* func_name,
                                       const std::string& content) {
  if (log_level > log_level_begin && log_level < log_level_end) {
    PlatformOutputLog(
        GenerateOutputLog(log_level, file_name, code_line, func_name, content));
  }
}

END_NAMESPACE_SHAREDLIB_NAME_BASE
