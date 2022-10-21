// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "dui_window.h"
#include "opencv_wrapper_pop_window.h"
#include "opencv_wrapper_window.h"

#include <string>

std::wstring Utf8ToUtf16(const std::string &utf8_str);

class OpencvWrapperFrame : public DuiWindow {
 public:
  OpencvWrapperFrame();
  ~OpencvWrapperFrame();

 private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("OpencvWrapperFrame");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
  bool OnCommand(WPARAM w_param, LPARAM l_param, LRESULT &l_res);  // NOLINT

  void Init();

 private:
  OpencvWrapperWindow *child_;
  OpencvWrapperPopWindow *pop_;
};
