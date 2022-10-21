// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "opencv_wrapper/opencv_wrapper_export.h"

BEGIN_NAMESPACE_OPENCV_WRAPPER

OPENCV_WRAPPER_API const char * GetVersion();
OPENCV_WRAPPER_API const char * GetOsName();
OPENCV_WRAPPER_API const char * GetName();

OPENCV_WRAPPER_API void ExtractVideoToFrames(const char* video_file_path, const char* output_dir);

END_NAMESPACE_OPENCV_WRAPPER
