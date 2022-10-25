// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ffmpeg_wrapper_frame.h"

#include "resource.h"
#include "ffmpeg_wrapper_rich_edit_window.h"
#include "ffmpeg_wrapper/ffmpeg_wrapper.h"

#define ID_VIEW_EXIT 9001
#define ID_HELP_ABOUT 9002
#define ID_VIEW_POP 9003

FfmpegWrapperFrame::FfmpegWrapperFrame() : child_(nullptr), pop_(nullptr) {}
FfmpegWrapperFrame::~FfmpegWrapperFrame() {}

LRESULT FfmpegWrapperFrame::HandleMessage(UINT msg, WPARAM w_param,
                                          LPARAM l_param) {
  switch (msg) {
    case WM_CREATE: {
      SetIcon(IDR_MAINFRAME);
      Init();
    } break;
    case WM_DESTROY: {
      ::PostQuitMessage(0L);
    } break;
    case WM_SETFOCUS: {
      if (child_) {
        ::SetFocus(child_->GetHwnd());
      }
    } break;
    case WM_SIZE: {
      RECT client_area;
      GetClientRect(GetHwnd(), &client_area);
      child_->SetLocation(0, 0);
      child_->SetSize(client_area.right - client_area.left,
                      client_area.bottom - client_area.top);
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
      ::BeginPaint(GetHwnd(), &ps);
      ::EndPaint(GetHwnd(), &ps);
      return 0;
    } break;
    case WM_COMMAND: {
      LRESULT l_res;
      if (OnCommand(w_param, l_param, l_res)) return l_res;
    } break;
    default:
      break;
  }
  return DuiWindow::HandleMessage(msg, w_param, l_param);
}
bool FfmpegWrapperFrame::OnCommand(WPARAM w_param, LPARAM l_param,
                                   LRESULT& l_res) {  // NOLINT
  switch (LOWORD(w_param)) {
    case ID_HELP_ABOUT: {
      std::wstring version = Utf8ToUtf16(FFMPEG_WRAPPER::GetVersion());
      ::MessageBox(GetHwnd(), version.c_str(), TEXT("FfmpegWrapperApp"),
                   MB_ICONINFORMATION);
    } break;
    case ID_VIEW_EXIT: {
      Close();
      return true;
    } break;
    case ID_VIEW_POP: {
      if (pop_) {
        pop_->Show(true);
        ::SetFocus(pop_->GetHwnd());
        return true;
      }
      // pop window
      pop_ = new FfmpegWrapperPopWindow();
      const UINT pop_style = WS_POPUP | WS_BORDER | WS_VISIBLE;
      RECT client_area;
      GetClientRect(GetHwnd(), &client_area);
      POINT left_top = {client_area.left, client_area.top};
      int width = client_area.right - client_area.left;
      int height = client_area.bottom - client_area.top;
      ::ClientToScreen(GetHwnd(), &left_top);
      width /= 2;
      height /= 2;
      left_top.x += width / 2;
      left_top.y += height / 2;
      pop_->Create(nullptr, TEXT("FfmpegWrapperPopWindow"), pop_style, 0,
                   left_top.x, left_top.y, width, height);
      pop_->Show(true);
      return true;
    } break;
    default:
      break;
  }
  return false;
}

void FfmpegWrapperFrame::Init() {
  // process child window
  child_ = new FfmpegWrapperRichEditWindow();
  const UINT child_style = ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
  child_->Create(GetHwnd(), TEXT(""), child_style, 0, 0, 0, 0, 0);
  // process menu
  HMENU hmenu = CreateMenu();
  HMENU sub_menu = CreatePopupMenu();
  AppendMenu(sub_menu, MF_STRING, ID_VIEW_EXIT, TEXT("Exit"));
  AppendMenu(sub_menu, MF_STRING, ID_VIEW_POP, TEXT("Pop"));
  AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT)sub_menu, TEXT("View"));
  sub_menu = CreatePopupMenu();
  AppendMenu(sub_menu, MF_STRING, ID_HELP_ABOUT, TEXT("About"));
  AppendMenu(hmenu, MF_STRING | MF_POPUP, (UINT)sub_menu, TEXT("Help"));
  SetMenu(GetHwnd(), hmenu);
}
