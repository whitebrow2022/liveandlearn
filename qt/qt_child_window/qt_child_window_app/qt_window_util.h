// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <QWidget>

class QtWindowUtil {
 public:
  static void AttachChildToParent(QWidget* parent, QWidget* child);
  static void DetachChildFromParent(QWidget* parent, QWidget* child);
  static void BeginSheet(QWidget* src_wgt, QWidget* sheet_wgt);
  static void EndSheet(QWidget* src_wgt, QWidget* sheet_wgt);
};
