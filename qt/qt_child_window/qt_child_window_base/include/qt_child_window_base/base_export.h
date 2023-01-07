// Created by liangxu on 2023/01/05.
//
// Copyright (c) 2023 The QtChildWindow Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define QT_CHILD_WINDOW_BASE_COMPONENT_BUILD
#if defined(QT_CHILD_WINDOW_BASE_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(QT_CHILD_WINDOW_BASE_IMPLEMENTATION)
#define QT_CHILD_WINDOW_BASE_EXPORT __declspec(dllexport)
#else
#define QT_CHILD_WINDOW_BASE_EXPORT __declspec(dllimport)
#endif  // defined(QT_CHILD_WINDOW_BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(QT_CHILD_WINDOW_BASE_IMPLEMENTATION)
#define QT_CHILD_WINDOW_BASE_EXPORT __attribute__((visibility("default")))
#else
#define QT_CHILD_WINDOW_BASE_EXPORT
#endif  // defined(QT_CHILD_WINDOW_BASE_IMPLEMENTATION)
#endif

#else  // defined(QT_CHILD_WINDOW_BASE_COMPONENT_BUILD)
#define QT_CHILD_WINDOW_BASE_EXPORT
#endif

#define QT_CHILD_WINDOW_BASE_API QT_CHILD_WINDOW_BASE_EXPORT

#define BEGIN_NAMESPACE_QT_CHILD_WINDOW_BASE namespace qt_child_window_base {
#define END_NAMESPACE_QT_CHILD_WINDOW_BASE }
#define QT_CHILD_WINDOW_BASE qt_child_window_base
