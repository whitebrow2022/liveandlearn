// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Windows.h>

class DuiApplication final {
 public:
  static bool TranslateMessage(const MSG& msg);
  static HINSTANCE GetResourceInstance();
  static void SetResourceInstance(HINSTANCE inst);
  static void MessageLoop();

 private:
  static HINSTANCE instance_;
};
