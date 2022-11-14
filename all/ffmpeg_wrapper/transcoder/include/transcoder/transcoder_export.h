// Created by liangxu on 2022/11/14.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#if defined(WIN32)

#if defined(TRANSCODER_IMPLEMENTATION)
#define TRANSCODER_EXPORT __declspec(dllexport)
#else
#define TRANSCODER_EXPORT __declspec(dllimport)
#endif  // defined(TRANSCODER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(TRANSCODER_IMPLEMENTATION)
#define TRANSCODER_EXPORT __attribute__((visibility("default")))
#else
#define TRANSCODER_EXPORT
#endif  // defined(TRANSCODER_IMPLEMENTATION)
#endif

#define TRANSCODER_API TRANSCODER_EXPORT
