// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define SHAREDLIB_NAME_BASE_COMPONENT_BUILD
#if defined(SHAREDLIB_NAME_BASE_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(SHAREDLIB_NAME_BASE_IMPLEMENTATION)
#define SHAREDLIB_NAME_BASE_EXPORT __declspec(dllexport)
#else
#define SHAREDLIB_NAME_BASE_EXPORT __declspec(dllimport)
#endif  // defined(SHAREDLIB_NAME_BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(SHAREDLIB_NAME_BASE_IMPLEMENTATION)
#define SHAREDLIB_NAME_BASE_EXPORT __attribute__((visibility("default")))
#else
#define SHAREDLIB_NAME_BASE_EXPORT
#endif  // defined(SHAREDLIB_NAME_BASE_IMPLEMENTATION)
#endif

#else  // defined(SHAREDLIB_NAME_BASE_COMPONENT_BUILD)
#define SHAREDLIB_NAME_BASE_EXPORT
#endif

#define SHAREDLIB_NAME_BASE_API SHAREDLIB_NAME_BASE_EXPORT

#define BEGIN_NAMESPACE_SHAREDLIB_NAME_BASE namespace sharedlib_name_base {
#define END_NAMESPACE_SHAREDLIB_NAME_BASE }
#define SHAREDLIB_NAME_BASE sharedlib_name_base
