// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

extern "C" {

int ConvertVideoToRawVideoAndAudio(const char* src_filename,
                                   const char* video_dst_filename,
                                   const char* audio_dst_filename);

}
