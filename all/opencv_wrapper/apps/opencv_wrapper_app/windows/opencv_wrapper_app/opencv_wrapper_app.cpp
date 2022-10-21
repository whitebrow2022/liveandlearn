// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// opencv_wrapper_app.cpp : Defines the entry point for the application.
//

#include "dui_application.h"
#include "opencv_wrapper_frame.h"
#include "stdafx.h"

int WINAPI WinMain(_In_ HINSTANCE inst,
                   _In_opt_ HINSTANCE HINSTANCE /*prev_instance*/,
                   _In_ LPSTR /*cmd_line*/, _In_ int cmd_show) {
  DuiApplication::SetResourceInstance(inst);

  HRESULT hr = ::CoInitialize(nullptr);
  if (FAILED(hr)) return 0;

  OpencvWrapperFrame* frame = new OpencvWrapperFrame();
  if (frame == nullptr) return 0;
  frame->Create(nullptr, TEXT("OpencvWrapperApp"),
                WS_VISIBLE | WS_OVERLAPPEDWINDOW, WS_EX_WINDOWEDGE);

  DuiApplication::MessageLoop();

  ::CoUninitialize();
  return 0;
}
