// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "dui_window.h"
#include "sharedlib_name_pop_window.h"
#include "sharedlib_name_window.h"

#include <string>

std::wstring Utf8ToUtf16(const std::string &utf8_str);

class SharedlibNameFrame : public DuiWindow {
 public:
  SharedlibNameFrame();
  ~SharedlibNameFrame();

 private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("SharedlibNameFrame");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
  bool OnCommand(WPARAM w_param, LPARAM l_param, LRESULT &l_res);  // NOLINT

  void Init();

 private:
  SharedlibNameWindow *child_;
  SharedlibNamePopWindow *pop_;
};
