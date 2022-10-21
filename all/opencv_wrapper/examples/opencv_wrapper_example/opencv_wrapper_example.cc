// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "opencv_wrapper/opencv_wrapper.h"

int main() {
  std::cout << OPENCV_WRAPPER::GetOsName() << " " << OPENCV_WRAPPER::GetName()
            << " " << OPENCV_WRAPPER::GetVersion() << std::endl;
  return 0;
}
