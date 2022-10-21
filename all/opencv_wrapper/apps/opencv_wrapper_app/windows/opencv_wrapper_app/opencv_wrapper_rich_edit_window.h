// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "opencv_wrapper_window.h"

class OpencvWrapperRichEditWindow : public OpencvWrapperWindow {
public:
  OpencvWrapperRichEditWindow();
  ~OpencvWrapperRichEditWindow();

private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("OpencvWrapperRichEditWindow");
  }
  LPCTSTR GetSuperClassName() const override;
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
