// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#if defined(OPENCV_WRAPPER_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(OPENCV_WRAPPER_IMPLEMENTATION)
#define OPENCV_WRAPPER_EXPORT __declspec(dllexport)
#else
#define OPENCV_WRAPPER_EXPORT __declspec(dllimport)
#endif  // defined(OPENCV_WRAPPER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(OPENCV_WRAPPER_IMPLEMENTATION)
#define OPENCV_WRAPPER_EXPORT __attribute__((visibility("default")))
#else
#define OPENCV_WRAPPER_EXPORT
#endif  // defined(OPENCV_WRAPPER_IMPLEMENTATION)
#endif

#else  // defined(OPENCV_WRAPPER_COMPONENT_BUILD)
#define OPENCV_WRAPPER_EXPORT
#endif

#define OPENCV_WRAPPER_API OPENCV_WRAPPER_EXPORT

#define BEGIN_NAMESPACE_OPENCV_WRAPPER namespace opencv_wrapper {
#define END_NAMESPACE_OPENCV_WRAPPER }
#define OPENCV_WRAPPER opencv_wrapper
