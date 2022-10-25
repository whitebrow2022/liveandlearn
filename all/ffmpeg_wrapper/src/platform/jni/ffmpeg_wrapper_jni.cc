// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include <string>

#include "ffmpeg_wrapper/ffmpeg_wrapper.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ffmpegwrapper_FfmpegWrapper_getVersion(JNIEnv* env,
                                                        jobject /* this */) {
  std::string version = FFMPEG_WRAPPER::GetVersion();
  return env->NewStringUTF(version.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ffmpegwrapper_FfmpegWrapper_getOsName(JNIEnv* env,
                                                       jobject /* this */) {
  std::string os_name = FFMPEG_WRAPPER::GetOsName();
  return env->NewStringUTF(os_name.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ffmpegwrapper_FfmpegWrapper_getName(JNIEnv* env,
                                                     jobject /* this */) {
  std::string lib_name = FFMPEG_WRAPPER::GetName();
  return env->NewStringUTF(lib_name.c_str());
}
