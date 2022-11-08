// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ffmpeg_wrapper/ffmpeg_wrapper.h"

#include "ffmpeg_video_converter.h"
#include "tests/demuxing_decoding.h"
#include "tests/hello_world.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/display.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

BEGIN_NAMESPACE_FFMPEG_WRAPPER

FFMPEG_WRAPPER_API const char *GetVersion() { return "1.0.0.001"; }
FFMPEG_WRAPPER_API const char *GetOsName() {
#if defined(OS_WINDOWS)
  return "Windows";
#elif defined(OS_ANDROID)
  return "Android";
#else
  return "Unknown system type";
#endif
}
FFMPEG_WRAPPER_API const char *GetName() { return "FfmpegWrapper"; }

FFMPEG_WRAPPER_API bool ConvertVideoToRawVideoAndRawAudio(
    const char *src_filename, const char *video_dst_filename,
    const char *audio_dst_filename) {
#if 0
  return ConvertVideoToRawVideoAndAudio(src_filename, video_dst_filename,
                                        audio_dst_filename) == 0;
#else
  return TestHelloWorld(src_filename) == 0;
#endif
}

END_NAMESPACE_FFMPEG_WRAPPER
