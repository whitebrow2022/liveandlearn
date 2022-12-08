// Created by liangxu on 2022/12/08.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define TRANSCODER_BASE_COMPONENT_BUILD
#if defined(TRANSCODER_BASE_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TRANSCODER_BASE_IMPLEMENTATION)
#define TRANSCODER_BASE_EXPORT __declspec(dllexport)
#else
#define TRANSCODER_BASE_EXPORT __declspec(dllimport)
#endif  // defined(TRANSCODER_BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(TRANSCODER_BASE_IMPLEMENTATION)
#define TRANSCODER_BASE_EXPORT __attribute__((visibility("default")))
#else
#define TRANSCODER_BASE_EXPORT
#endif  // defined(TRANSCODER_BASE_IMPLEMENTATION)
#endif

#else  // defined(TRANSCODER_BASE_COMPONENT_BUILD)
#define TRANSCODER_BASE_EXPORT
#endif

#define TRANSCODER_BASE_API TRANSCODER_BASE_EXPORT

#define BEGIN_NAMESPACE_TRANSCODER_BASE namespace transcoder_base {
#define END_NAMESPACE_TRANSCODER_BASE }
#define TRANSCODER_BASE transcoder_base
