// Created by liangxu on 2022/11/28.
//
// Copyright (c) 2022 The MicrosoftCpp Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <iostream>

#include "base/base_paths.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"

// namespace {
// base::FilePath GetModuleDir() {
//   base::FilePath exe_dir;
//   const bool has_path = base::PathService::Get(base::DIR_EXE, &exe_dir);
//   DCHECK(has_path);
//   return exe_dir;
// }
// }  // namespace

// int main(int argc, char* argv[]) {
//   printf("Hello from the base_demo.\n");
//   auto module_dir = GetModuleDir();
//   std::cout << module_dir.AsUTF8Unsafe();
//   return 0;
// }
