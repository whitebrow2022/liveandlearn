// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper_pop_window.h"

#include "resource.h"

OpencvWrapperPopWindow::OpencvWrapperPopWindow() {}
OpencvWrapperPopWindow::~OpencvWrapperPopWindow() {}

LRESULT OpencvWrapperPopWindow::HandleMessage(UINT msg, WPARAM w_param,
                                              LPARAM l_param) {
  switch (msg) {
    case WM_CREATE: {
      SetIcon(IDR_MAINFRAME);
    } break;
    case WM_DESTROY: {
    } break;
    case WM_SETFOCUS: {
    } break;
    case WM_SIZE: {
      return 0;
    } break;
    case WM_ERASEBKGND: {
      HDC hdc = (HDC)w_param;
      RECT erase_area;
      GetClientRect(GetHwnd(), &erase_area);
      FillRect(hdc, &erase_area, (HBRUSH)GetStockObject(WHITE_BRUSH));
      return 1;
    } break;
    case WM_PAINT: {
      PAINTSTRUCT ps = {0};
      HDC hdc = ::BeginPaint(GetHwnd(), &ps);
      TCHAR text[] = TEXT("OpencvWrapperPopWindow");
      RECT client_area;
      GetClientRect(GetHwnd(), &client_area);
      ::DrawText(hdc, text, sizeof(text) / sizeof(TCHAR) - 1, &client_area,
                 DT_CENTER | DT_VCENTER);
      ::EndPaint(GetHwnd(), &ps);
      return 0;
    } break;
  }
  return DuiWindow::HandleMessage(msg, w_param, l_param);
}
