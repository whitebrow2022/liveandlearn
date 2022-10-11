// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once


#if defined(SHAREDLIB_NAME_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(SHAREDLIB_NAME_IMPLEMENTATION)
#define SHAREDLIB_NAME_EXPORT __declspec(dllexport)
#else 
#define SHAREDLIB_NAME_EXPORT __declspec(dllimport)
#endif  // defined(SHAREDLIB_NAME_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(SHAREDLIB_NAME_IMPLEMENTATION)
#define SHAREDLIB_NAME_EXPORT __attribute__((visibility("default")))
#else 
#define SHAREDLIB_NAME_EXPORT 
#endif  // defined(SHAREDLIB_NAME_IMPLEMENTATION)
#endif

#else  // defined(SHAREDLIB_NAME_COMPONENT_BUILD)
#define SHAREDLIB_NAME_EXPORT
#endif


#define SHAREDLIB_NAME_API SHAREDLIB_NAME_EXPORT


#define BEGIN_NAMESPACE_SHAREDLIB_NAME namespace sharedlib_name {
#define END_NAMESPACE_SHAREDLIB_NAME }
#define SHAREDLIB_NAME sharedlib_name
