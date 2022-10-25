// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "ffmpeg_wrapper/ffmpeg_wrapper.h"

int main() {
  std::cout << FFMPEG_WRAPPER::GetOsName() << " " << FFMPEG_WRAPPER::GetName()
            << " " << FFMPEG_WRAPPER::GetVersion() << std::endl;
  return 0;
}
