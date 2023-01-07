// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qt_window_util.h"

#import <AppKit/AppKit.h>

#include "qt_child_window_base/log/log_writer.h"

namespace {
NSWindow* GetNSWindowFromQWidget(QWidget* wgt) {
  if (!wgt) {
    return nullptr;
  }
  NSView* nsview = (__bridge NSView *)reinterpret_cast<void*>(wgt->winId());
  NSWindow* nswindow = [nsview window];
  if (!nswindow) {
    return nullptr;
  }
  return nswindow;
}
}

void QtWindowUtil::AttachChildToParent(QWidget* parent, QWidget* child) {
  NSWindow *nsparent = GetNSWindowFromQWidget(parent);
  NSWindow *nschild = GetNSWindowFromQWidget(child);
  if (!nsparent || !nschild || nsparent == nschild) {
    return;
  }
  [nsparent addChildWindow:nschild ordered:NSWindowAbove];
}
void QtWindowUtil::DetachChildFromParent(QWidget* parent, QWidget* child) {
  NSWindow *nsparent = GetNSWindowFromQWidget(parent);
  NSWindow *nschild = GetNSWindowFromQWidget(child);
  if (!nsparent || !nschild || nsparent == nschild) {
    return;
  }
  [nsparent removeChildWindow:nschild];
}

void QtWindowUtil::BeginSheet(QWidget* parent, QWidget* child) {
  NSWindow *nsparent = GetNSWindowFromQWidget(parent);
  NSWindow *nschild = GetNSWindowFromQWidget(child);
  if (!nsparent || !nschild || nsparent == nschild) {
    return;
  }
  static uint32_t s_sheet_count = 0;
  __block uint32_t curr_sheet_count = s_sheet_count++;
  //
  log_info << curr_sheet_count << " about to beginSheet";
  //sheet_window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
  [nsparent beginSheet:nschild
       completionHandler:^(NSModalResponse ret_code){
         log_info << curr_sheet_count << " beginSheet finished: " << static_cast<int>(ret_code);
       }];
}
void QtWindowUtil::EndSheet(QWidget* parent, QWidget* child) {
  NSWindow *nsparent = GetNSWindowFromQWidget(parent);
  NSWindow *nschild = GetNSWindowFromQWidget(child);
  if (!nsparent || !nschild || nsparent == nschild) {
    return;
  }
  [nsparent endSheet:nschild];
}
