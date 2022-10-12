// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "dui_window.h"

class SharedlibNamePopWindow : public DuiWindow {
 public:
  SharedlibNamePopWindow();
  ~SharedlibNamePopWindow();

 private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("SharedlibNamePopWindow");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
