// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name/sharedlib_name.h"

BEGIN_NAMESPACE_SHAREDLIB_NAME

SHAREDLIB_NAME_API const char* GetVersion() { return "1.0.0.001"; }
SHAREDLIB_NAME_API const char* GetOsName() {
#if defined(OS_WINDOWS)
  return "Windows";
#elif defined(OS_ANDROID)
  return "Android";
#else
  return "Unknown system type";
#endif
}
SHAREDLIB_NAME_API const char* GetName() { return "SharedlibName"; }

END_NAMESPACE_SHAREDLIB_NAME
