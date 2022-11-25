// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

#include "hash_generator_base/base_export.h"

template <typename T>
struct is_char_ptr : std::false_type {};
template <>
struct is_char_ptr<const char*> : std::true_type {};
template <>
struct is_char_ptr<char*> : std::true_type {};

template <typename T>
struct is_null_ptr {
  static bool is(const T& obj) { return false; }
};
template <typename T>
struct is_null_ptr<T*> {
  static bool is(const T* obj) { return obj == nullptr; }
};

using OutputLogFuncType = std::function<void(
    int32_t log_level, const char* file_name, int32_t code_line,
    const char* func_name, const std::string& content)>;

class AnonymousLogWriter final {
 public:
  AnonymousLogWriter(int32_t log_level, const char* file_name,
                     int32_t code_line, const char* func_name,
                     OutputLogFuncType output)
      : log_level_(log_level),
        file_name_(file_name),
        code_line_(code_line),
        func_name_(func_name),
        output_(output) {}
  ~AnonymousLogWriter() {
    if (output_) {
      output_(log_level_, file_name_.c_str(), code_line_, func_name_.c_str(),
              oss_.str());
    }
  }

 public:
  template <typename T,
            typename = typename std::enable_if<!std::is_enum<T>::value>::type>
  inline AnonymousLogWriter& operator<<(const T& obj) {
    if (std::is_pointer<T>::value) {
      if (is_null_ptr<T>::is(obj)) {
        oss_ << "nullptr";
      } else {
        if (is_char_ptr<T>::value) {
          oss_ << obj;
        } else {
          auto old_flags = oss_.flags();
          oss_ << std::hex << obj;
          oss_.flags(old_flags);
        }
      }
    } else {
      oss_ << obj;
    }
    return *this;
  }
  inline AnonymousLogWriter& operator<<(std::ostream& (*obj)(std::ostream&)) {
    oss_ << obj;
    return *this;
  }
  template <typename T,
            typename = typename std::enable_if<std::is_enum<T>::value>::type>
  inline AnonymousLogWriter& operator<<(T v) {
    oss_ << static_cast<typename std::underlying_type<T>::type>(v);
    return *this;
  }

 private:
  std::ostringstream oss_;
  int32_t log_level_;
  std::string file_name_;
  int32_t code_line_;
  std::string func_name_;
  OutputLogFuncType output_;
};

#define log_level_begin 0x00
#define log_level_fault 0x01
#define log_level_cpe 0x02
#define log_level_error 0x03
#define log_level_warning 0x04
#define log_level_cp 0x05
#define log_level_info 0x06
#define log_level_debug 0x07
#define log_level_end 0x08
