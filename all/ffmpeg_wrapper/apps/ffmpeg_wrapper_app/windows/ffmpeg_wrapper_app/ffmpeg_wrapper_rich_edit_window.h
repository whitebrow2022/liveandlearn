// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "ffmpeg_wrapper_window.h"

class FfmpegWrapperRichEditWindow : public FfmpegWrapperWindow {
public:
  FfmpegWrapperRichEditWindow();
  ~FfmpegWrapperRichEditWindow();

private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("FfmpegWrapperRichEditWindow");
  }
  LPCTSTR GetSuperClassName() const override;
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
