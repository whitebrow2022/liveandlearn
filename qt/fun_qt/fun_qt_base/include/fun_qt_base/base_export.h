// Created by liangxu on 2023/01/16.
//
// Copyright (c) 2023 The FunQt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define FUN_QT_BASE_COMPONENT_BUILD
#if defined(FUN_QT_BASE_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(FUN_QT_BASE_IMPLEMENTATION)
#define FUN_QT_BASE_EXPORT __declspec(dllexport)
#else
#define FUN_QT_BASE_EXPORT __declspec(dllimport)
#endif  // defined(FUN_QT_BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(FUN_QT_BASE_IMPLEMENTATION)
#define FUN_QT_BASE_EXPORT __attribute__((visibility("default")))
#else
#define FUN_QT_BASE_EXPORT
#endif  // defined(FUN_QT_BASE_IMPLEMENTATION)
#endif

#else  // defined(FUN_QT_BASE_COMPONENT_BUILD)
#define FUN_QT_BASE_EXPORT
#endif

#define FUN_QT_BASE_API FUN_QT_BASE_EXPORT

#define BEGIN_NAMESPACE_FUN_QT_BASE namespace fun_qt_base {
#define END_NAMESPACE_FUN_QT_BASE }
#define FUN_QT_BASE fun_qt_base
