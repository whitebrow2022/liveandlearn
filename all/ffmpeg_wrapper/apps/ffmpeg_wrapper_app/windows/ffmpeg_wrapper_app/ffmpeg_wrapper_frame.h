// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "dui_window.h"
#include "ffmpeg_wrapper_pop_window.h"
#include "ffmpeg_wrapper_window.h"

#include <string>

std::wstring Utf8ToUtf16(const std::string &utf8_str);

class FfmpegWrapperFrame : public DuiWindow {
 public:
  FfmpegWrapperFrame();
  ~FfmpegWrapperFrame();

 private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("FfmpegWrapperFrame");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
  bool OnCommand(WPARAM w_param, LPARAM l_param, LRESULT &l_res);  // NOLINT

  void Init();

 private:
  FfmpegWrapperWindow *child_;
  FfmpegWrapperPopWindow *pop_;
};
