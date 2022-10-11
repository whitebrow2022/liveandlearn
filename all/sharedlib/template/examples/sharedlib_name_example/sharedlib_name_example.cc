// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sharedlib_name/sharedlib_name.h"

#include <iostream>

int main() {
  std::cout << SHAREDLIB_NAME::GetOsName() << " " << SHAREDLIB_NAME::GetName() << " " << SHAREDLIB_NAME::GetVersion() << std::endl;
  return 0;
}
