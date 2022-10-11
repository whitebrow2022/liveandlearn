# Created by %username% on %date%.
#
# Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 
# Android: https://developer.android.com/studio/projects/install-ndk
# http://romannurik.github.io/AndroidAssetStudio/

cmake_minimum_required(VERSION 3.12)

set(project_name sharedlib_name)

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
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/sharedlib_name.cmake)

add_definitions(-DOS_ANDROID)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/out)

file(GLOB_RECURSE sharedlib_name_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc ${sharedlib_name_src})

file(GLOB_RECURSE jni_src
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/platform/*.h
  )
list(APPEND sharedlib_name_src ${jni_src})

add_library(sharedlib_name SHARED ${sharedlib_name_src})
target_link_libraries(sharedlib_name ${common_name})
