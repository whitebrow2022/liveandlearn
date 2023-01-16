// Created by liangxu on 2023/01/16.
//
// Copyright (c) 2023 The FunQt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "fun_qt_base/base_export.h"
#include "fun_qt_base/log/logger.h"

BEGIN_NAMESPACE_FUN_QT_BASE

FUN_QT_BASE_API void InitLog(const char* log_path);
FUN_QT_BASE_API void UnInitLog();

FUN_QT_BASE_API int32_t GetLogLevel();
FUN_QT_BASE_API void OutputLog(int32_t log_level, const char* file_name,
                                       int32_t code_line, const char* func_name,
                                       const std::string& content);

END_NAMESPACE_FUN_QT_BASE

#define log_content(log_level)                                                \
  if (fun_qt_base::GetLogLevel() >= log_level_##log_level)            \
  AnonymousLogWriter(log_level_##log_level, __FILE__, __LINE__, __FUNCTION__, \
                     fun_qt_base::OutputLog)

#define log_fault log_content(fault)
#define log_warning log_content(warning)
#define log_info log_content(info)
#define log_debug log_content(debug)
