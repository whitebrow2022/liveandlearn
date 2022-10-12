// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include <string>

#include "sharedlib_name/sharedlib_name.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_sharedlibname_SharedlibName_getVersion(JNIEnv* env,
                                                        jobject /* this */) {
  std::string version = SHAREDLIB_NAME::GetVersion();
  return env->NewStringUTF(version.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_sharedlibname_SharedlibName_getOsName(JNIEnv* env,
                                                       jobject /* this */) {
  std::string os_name = SHAREDLIB_NAME::GetOsName();
  return env->NewStringUTF(os_name.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_sharedlibname_SharedlibName_getName(JNIEnv* env,
                                                     jobject /* this */) {
  std::string lib_name = SHAREDLIB_NAME::GetName();
  return env->NewStringUTF(lib_name.c_str());
}
