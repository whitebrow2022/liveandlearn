// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "qt_child_window_base/base_export.h"
#include "qt_child_window_base/log/logger.h"

BEGIN_NAMESPACE_QT_CHILD_WINDOW_BASE

QT_CHILD_WINDOW_BASE_API void InitLog(const char* log_path);
QT_CHILD_WINDOW_BASE_API void UnInitLog();

QT_CHILD_WINDOW_BASE_API int32_t GetLogLevel();
QT_CHILD_WINDOW_BASE_API void OutputLog(int32_t log_level,
                                        const char* file_name,
                                        int32_t code_line,
                                        const char* func_name,
                                        const std::string& content);

END_NAMESPACE_QT_CHILD_WINDOW_BASE

#define log_content(log_level)                                                \
  if (qt_child_window_base::GetLogLevel() >= log_level_##log_level)           \
  AnonymousLogWriter(log_level_##log_level, __FILE__, __LINE__, __FUNCTION__, \
                     qt_child_window_base::OutputLog)

#define log_fault log_content(fault)
#define log_warning log_content(warning)
#define log_info log_content(info)
#define log_debug log_content(debug)
