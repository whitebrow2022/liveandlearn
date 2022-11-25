// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#define HASH_GENERATOR_BASE_COMPONENT_BUILD
#if defined(HASH_GENERATOR_BASE_COMPONENT_BUILD)
#if defined(WIN32)

#if defined(HASH_GENERATOR_BASE_IMPLEMENTATION)
#define HASH_GENERATOR_BASE_EXPORT __declspec(dllexport)
#else
#define HASH_GENERATOR_BASE_EXPORT __declspec(dllimport)
#endif  // defined(HASH_GENERATOR_BASE_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(HASH_GENERATOR_BASE_IMPLEMENTATION)
#define HASH_GENERATOR_BASE_EXPORT __attribute__((visibility("default")))
#else
#define HASH_GENERATOR_BASE_EXPORT
#endif  // defined(HASH_GENERATOR_BASE_IMPLEMENTATION)
#endif

#else  // defined(HASH_GENERATOR_BASE_COMPONENT_BUILD)
#define HASH_GENERATOR_BASE_EXPORT
#endif

#define HASH_GENERATOR_BASE_API HASH_GENERATOR_BASE_EXPORT

#define BEGIN_NAMESPACE_HASH_GENERATOR_BASE namespace hash_generator_base {
#define END_NAMESPACE_HASH_GENERATOR_BASE }
#define HASH_GENERATOR_BASE hash_generator_base
