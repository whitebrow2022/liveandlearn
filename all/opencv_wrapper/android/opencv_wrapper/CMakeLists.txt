# Created by liangxu on 2022/10/21.
#
# Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 
# Android: https://developer.android.com/studio/projects/install-ndk
# http://romannurik.github.io/AndroidAssetStudio/

cmake_minimum_required(VERSION 3.12)

set(project_name opencv_wrapper)

project(${project_name})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Separate multiple Projects and put them into folders which are on top-level.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# How do I make CMake output into a 'bin' dir?
#   The correct variable to set is CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#   We use the following in our root CMakeLists.txt:
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/opencv_wrapper.cmake)

add_definitions(-DOS_ANDROID)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/out)

file(GLOB_RECURSE opencv_wrapper_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc ${opencv_wrapper_src})

file(GLOB_RECURSE jni_src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/*.h
  )
list(APPEND opencv_wrapper_src ${jni_src})

add_library(opencv_wrapper SHARED ${opencv_wrapper_src})
target_link_libraries(opencv_wrapper ${common_name})
