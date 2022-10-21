// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include <string>

#include "opencv_wrapper/opencv_wrapper.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_opencvwrapper_OpencvWrapper_getVersion(JNIEnv* env,
                                                        jobject /* this */) {
  std::string version = OPENCV_WRAPPER::GetVersion();
  return env->NewStringUTF(version.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_opencvwrapper_OpencvWrapper_getOsName(JNIEnv* env,
                                                       jobject /* this */) {
  std::string os_name = OPENCV_WRAPPER::GetOsName();
  return env->NewStringUTF(os_name.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_opencvwrapper_OpencvWrapper_getName(JNIEnv* env,
                                                     jobject /* this */) {
  std::string lib_name = OPENCV_WRAPPER::GetName();
  return env->NewStringUTF(lib_name.c_str());
}
