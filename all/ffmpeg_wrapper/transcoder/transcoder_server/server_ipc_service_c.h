// Created by liangxu on 2022/11/15.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
void WriteToClient(const char *utf8_data);
void AvLog(void *avcl, int level, const char *fmt, ...);
void PostStarted();
void PostProgress(double progress);
void PostStoped();
#ifdef __cplusplus
}
#endif
