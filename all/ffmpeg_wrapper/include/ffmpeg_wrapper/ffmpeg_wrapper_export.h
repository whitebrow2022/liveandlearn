// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#if defined(FFMPEG_WRAPPER_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(FFMPEG_WRAPPER_IMPLEMENTATION)
#define FFMPEG_WRAPPER_EXPORT __declspec(dllexport)
#else
#define FFMPEG_WRAPPER_EXPORT __declspec(dllimport)
#endif  // defined(FFMPEG_WRAPPER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(FFMPEG_WRAPPER_IMPLEMENTATION)
#define FFMPEG_WRAPPER_EXPORT __attribute__((visibility("default")))
#else
#define FFMPEG_WRAPPER_EXPORT
#endif  // defined(FFMPEG_WRAPPER_IMPLEMENTATION)
#endif

#else  // defined(FFMPEG_WRAPPER_COMPONENT_BUILD)
#define FFMPEG_WRAPPER_EXPORT
#endif

#define FFMPEG_WRAPPER_API FFMPEG_WRAPPER_EXPORT

#define BEGIN_NAMESPACE_FFMPEG_WRAPPER namespace ffmpeg_wrapper {
#define END_NAMESPACE_FFMPEG_WRAPPER }
#define FFMPEG_WRAPPER ffmpeg_wrapper
