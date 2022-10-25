// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dui_application.h"

HINSTANCE DuiApplication::instance_ = nullptr;

bool DuiApplication::TranslateMessage(const MSG& msg) { return false; }
HINSTANCE DuiApplication::GetResourceInstance() { return instance_; }
void DuiApplication::SetResourceInstance(HINSTANCE inst) { instance_ = inst; }
void DuiApplication::MessageLoop() {
  MSG msg = {0};
  while (::GetMessage(&msg, nullptr, 0, 0)) {
    if (!DuiApplication::TranslateMessage(msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }
}
