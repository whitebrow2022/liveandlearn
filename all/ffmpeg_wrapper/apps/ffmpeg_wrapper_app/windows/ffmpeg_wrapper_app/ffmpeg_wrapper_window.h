// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "dui_window.h"

class FfmpegWrapperWindow : public DuiWindow {
 public:
  FfmpegWrapperWindow();
  ~FfmpegWrapperWindow();

 protected:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("FfmpegWrapperWindow");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
