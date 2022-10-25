// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "ffmpeg_wrapper/ffmpeg_wrapper_export.h"

BEGIN_NAMESPACE_FFMPEG_WRAPPER

FFMPEG_WRAPPER_API const char* GetVersion();
FFMPEG_WRAPPER_API const char* GetOsName();
FFMPEG_WRAPPER_API const char* GetName();

FFMPEG_WRAPPER_API bool ConvertVideoToRawVideoAndRawAudio(
    const char* src_filename, const char* video_dst_filename,
    const char* audio_dst_filename);

END_NAMESPACE_FFMPEG_WRAPPER
