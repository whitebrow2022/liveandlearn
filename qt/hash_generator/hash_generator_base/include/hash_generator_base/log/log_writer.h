// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "hash_generator_base/base_export.h"
#include "hash_generator_base/log/logger.h"

BEGIN_NAMESPACE_HASH_GENERATOR_BASE

HASH_GENERATOR_BASE_API void InitLog(const char* log_path);
HASH_GENERATOR_BASE_API void UnInitLog();

HASH_GENERATOR_BASE_API int32_t GetLogLevel();
HASH_GENERATOR_BASE_API void OutputLog(int32_t log_level, const char* file_name,
                                       int32_t code_line, const char* func_name,
                                       const std::string& content);

END_NAMESPACE_HASH_GENERATOR_BASE

#define log_content(log_level)                                                \
  if (hash_generator_base::GetLogLevel() >= log_level_##log_level)            \
  AnonymousLogWriter(log_level_##log_level, __FILE__, __LINE__, __FUNCTION__, \
                     hash_generator_base::OutputLog)

#define log_fault log_content(fault)
#define log_warning log_content(warning)
#define log_info log_content(info)
#define log_debug log_content(debug)
