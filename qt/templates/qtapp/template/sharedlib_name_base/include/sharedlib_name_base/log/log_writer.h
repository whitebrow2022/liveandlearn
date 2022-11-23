// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "sharedlib_name_base/base_export.h"
#include "sharedlib_name_base/log/logger.h"

BEGIN_NAMESPACE_SHAREDLIB_NAME_BASE

SHAREDLIB_NAME_BASE_API int32_t GetLogLevel();
SHAREDLIB_NAME_BASE_API void OutputLog(int32_t log_level, const char* file_name,
                                       int32_t code_line, const char* func_name,
                                       const std::string& content);

END_NAMESPACE_SHAREDLIB_NAME_BASE

#define log_content(log_level)                                                \
  if (sharedlib_name_base::GetLogLevel() >= log_level_##log_level)            \
  AnonymousLogWriter(log_level_##log_level, __FILE__, __LINE__, __FUNCTION__, \
                     sharedlib_name_base::OutputLog)

#define log_fault log_content(fault)
#define log_warning log_content(warning)
#define log_info log_content(info)
#define log_debug log_content(debug)
