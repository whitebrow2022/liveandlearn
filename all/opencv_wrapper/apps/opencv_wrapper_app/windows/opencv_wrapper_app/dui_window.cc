// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dui_window.h"

#include <windowsx.h>

#include "dui_application.h"

#define ASSERT(exp) ((VOID)0)

DuiWindow::DuiWindow() : hwnd_(nullptr), old_wndproc_(::DefWindowProc) {}
DuiWindow::~DuiWindow() {}

HWND DuiWindow::GetHwnd() const { return hwnd_; }
DuiWindow::operator HWND() const { return hwnd_; }

bool DuiWindow::RegisterWindowClass() {
  WNDCLASS wc = {0};
  wc.style = GetClassStyle();
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIcon = nullptr;
  wc.lpfnWndProc = DuiWindow::DuiWindowProc;
  wc.hInstance = DuiApplication::GetResourceInstance();
  wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = GetWindowClassName();
  ATOM ret = ::RegisterClass(&wc);
  ASSERT(ret != 0 || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
  return ret != 0 || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}
bool DuiWindow::RegisterSuperclass() {
  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  if (!::GetClassInfoEx(nullptr, GetSuperClassName(), &wc)) {
    if (!::GetClassInfoEx(DuiApplication::GetResourceInstance(),
                          GetSuperClassName(), &wc)) {
      ASSERT(!"Unable to locate window class");
      return false;
    }
  }
  old_wndproc_ = wc.lpfnWndProc;
  wc.lpfnWndProc = DuiWindow::DuiControlProc;
  wc.hInstance = DuiApplication::GetResourceInstance();
  wc.lpszClassName = GetWindowClassName();
  ATOM ret = ::RegisterClassEx(&wc);
  ASSERT(ret != 0 || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
  return ret != 0 || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND DuiWindow::Create(HWND parent, LPCTSTR name, DWORD style, DWORD ex_style,
                       const RECT area, HMENU menu /* = nullptr*/) {
  return Create(parent, name, style, ex_style, area.left, area.top,
                area.right - area.left, area.bottom - area.top, menu);
}
HWND DuiWindow::Create(HWND parent, LPCTSTR name, DWORD style, DWORD ex_style,
                       int left /* = CW_USEDEFAULT*/,
                       int top /* = CW_USEDEFAULT*/,
                       int width /* = CW_USEDEFAULT*/,
                       int height /* = CW_USEDEFAULT*/,
                       HMENU menu /* = nullptr*/) {
  if (GetSuperClassName() != nullptr && !RegisterSuperclass()) return nullptr;
  if (GetSuperClassName() == nullptr && !RegisterWindowClass()) return nullptr;
  hwnd_ = ::CreateWindowEx(ex_style, GetWindowClassName(), name, style, left,
                           top, width, height, parent, menu,
                           DuiApplication::GetResourceInstance(), this);
  ASSERT(hwnd_ != nullptr);
  return hwnd_;
}

void DuiWindow::Show(bool take_focus /* = true*/) {
  ASSERT(::IsWindow(hwnd_));
  if (!::IsWindow(hwnd_)) return;
  ::ShowWindow(hwnd_, take_focus ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
}
void DuiWindow::Hide() {
  ASSERT(::IsWindow(hwnd_));
  if (!::IsWindow(hwnd_)) return;
  ::ShowWindow(hwnd_, SW_HIDE);
}
bool DuiWindow::ShowModal() {
  ASSERT(::IsWindow(hwnd_));
  HWND parent = ::GetWindow(hwnd_, GW_OWNER);
  ::ShowWindow(hwnd_, SW_SHOWNORMAL);
  ::EnableWindow(parent, FALSE);
  MSG msg = {0};
  while (::IsWindow(hwnd_) && ::GetMessage(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_CLOSE) {
      ::EnableWindow(parent, TRUE);
      ::SetFocus(parent);
    }
    if (!DuiApplication::TranslateMessage(msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    if (msg.message == WM_QUIT) break;
  }
  ::EnableWindow(parent, TRUE);
  ::SetFocus(parent);
  if (msg.message == WM_QUIT) ::PostQuitMessage(msg.wParam);
  return true;
}
void DuiWindow::Close() {
  ASSERT(::IsWindow(hwnd_));
  if (!::IsWindow(hwnd_)) return;
  ::PostMessage(hwnd_, WM_CLOSE, 0, 0);
}
void DuiWindow::Center() {}
void DuiWindow::SetIcon(UINT res) {
  HICON hicon = (HICON)::LoadImage(
      DuiApplication::GetResourceInstance(), MAKEINTRESOURCE(res), IMAGE_ICON,
      ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON),
      LR_DEFAULTCOLOR);
  ASSERT(hicon);
  ::SendMessage(hwnd_, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
  hicon = (HICON)::LoadImage(DuiApplication::GetResourceInstance(),
                             MAKEINTRESOURCE(res), IMAGE_ICON,
                             ::GetSystemMetrics(SM_CXSMICON),
                             ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
  ASSERT(hicon);
  ::SendMessage(hwnd_, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hicon);
}
void DuiWindow::SetLocation(int left, int top) {
  UINT flags = SWP_NOZORDER | SWP_NOSIZE;
  ::SetWindowPos(hwnd_, nullptr, left, top, 0, 0, flags);
}
void DuiWindow::SetSize(int width, int height) { ResizeClient(width, height); }

LPCTSTR DuiWindow::GetSuperClassName() const { return nullptr; }
UINT DuiWindow::GetClassStyle() const { return 0; }

void DuiWindow::ResizeClient(int width /* = -1*/, int height /* = -1*/) {
  ASSERT(::IsWindow(hwnd_));
  RECT client_area = {0};
  if (!::GetClientRect(hwnd_, &client_area)) return;
  if (width != -1) client_area.right = width;
  if (height != -1) client_area.bottom = height;
  if (!::AdjustWindowRectEx(&client_area, GetWindowStyle(hwnd_),
                            (!(GetWindowStyle(hwnd_) & WS_CHILD) &&
                             (::GetMenu(hwnd_) != nullptr)),
                            GetWindowExStyle(hwnd_)))
    return;
  UINT flags = SWP_NOZORDER | SWP_NOMOVE;
  ::SetWindowPos(hwnd_, nullptr, 0, 0, client_area.right - client_area.left,
                 client_area.bottom - client_area.top, flags);
}

LRESULT DuiWindow::HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) {
  return ::CallWindowProc(old_wndproc_, hwnd_, msg, w_param, l_param);
}
void DuiWindow::OnFinalMessage(HWND hwnd) {}

LRESULT CALLBACK DuiWindow::DuiWindowProc(HWND hwnd, UINT msg, WPARAM w_param,
                                          LPARAM l_param) {
  DuiWindow* this_ptr = nullptr;
  if (msg == WM_NCCREATE) {
    LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(l_param);
    this_ptr = static_cast<DuiWindow*>(lpcs->lpCreateParams);
    this_ptr->hwnd_ = hwnd;
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this_ptr));
  } else {
    this_ptr =
        reinterpret_cast<DuiWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_NCDESTROY && this_ptr != nullptr) {
      LRESULT l_res =
          ::CallWindowProc(this_ptr->old_wndproc_, hwnd, msg, w_param, l_param);
      ::SetWindowLongPtr(this_ptr->hwnd_, GWLP_USERDATA, 0L);
      this_ptr->hwnd_ = nullptr;
      this_ptr->OnFinalMessage(hwnd);
      return l_res;
    }
  }
  if (this_ptr != nullptr) {
    return this_ptr->HandleMessage(msg, w_param, l_param);
  } else {
    return ::DefWindowProc(hwnd, msg, w_param, l_param);
  }
}
LRESULT CALLBACK DuiWindow::DuiControlProc(HWND hwnd, UINT msg, WPARAM w_param,
                                           LPARAM l_param) {
  static const TCHAR dui_wnd_key[8] = TEXT("WndX");
  DuiWindow* this_ptr = nullptr;
  if (msg == WM_NCCREATE) {
    LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(l_param);
    this_ptr = static_cast<DuiWindow*>(lpcs->lpCreateParams);
    ::SetProp(hwnd, dui_wnd_key, (HANDLE)this_ptr);
    this_ptr->hwnd_ = hwnd;
  } else {
    this_ptr = reinterpret_cast<DuiWindow*>(::GetProp(hwnd, dui_wnd_key));
    if (msg == WM_NCDESTROY && this_ptr != nullptr) {
      LRESULT l_res =
          ::CallWindowProc(this_ptr->old_wndproc_, hwnd, msg, w_param, l_param);
      ::SetProp(hwnd, dui_wnd_key, nullptr);
      this_ptr->hwnd_ = nullptr;
      this_ptr->OnFinalMessage(hwnd);
      return l_res;
    }
  }
  if (this_ptr != nullptr) {
    return this_ptr->HandleMessage(msg, w_param, l_param);
  } else {
    return ::DefWindowProc(hwnd, msg, w_param, l_param);
  }
}
