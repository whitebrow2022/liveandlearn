// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper_rich_edit_window.h"

#include <Richedit.h>

#include <string>

OpencvWrapperRichEditWindow::OpencvWrapperRichEditWindow() {}
OpencvWrapperRichEditWindow::~OpencvWrapperRichEditWindow() {}

LPCTSTR OpencvWrapperRichEditWindow::GetSuperClassName() const {
  static std::wstring rich_edit_class_name;
  if (rich_edit_class_name.empty()) {
    if (::LoadLibrary(TEXT("Msftedit.dll"))) {
      rich_edit_class_name = MSFTEDIT_CLASS;
    } else if (::LoadLibrary(TEXT("Riched32.dll"))) {
      rich_edit_class_name = RICHEDIT_CLASS;
    } else if (::LoadLibrary(TEXT("Riched20.dll"))) {
      rich_edit_class_name = RICHEDIT_CLASS;
    }
  }
  if (rich_edit_class_name.empty()) {
    return nullptr;
  }
  return rich_edit_class_name.c_str();
}
LRESULT OpencvWrapperRichEditWindow::HandleMessage(UINT msg, WPARAM w_param,
                                                   LPARAM l_param) {
  return DuiWindow::HandleMessage(msg, w_param, l_param);
}
