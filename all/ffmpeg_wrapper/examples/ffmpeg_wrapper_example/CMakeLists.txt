# Created by liangxu on 2022/10/25.
#
# Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.20)

set(project_name ffmpeg_wrapper_example)

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
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ffmpeg_wrapper.cmake)

if (WINDOWS)
  add_definitions(-DOS_WINDOWS)
elseif(ANDROID)
  add_definitions(-DOS_ANDROID)
elseif(MACOS)
  add_definitions(-DOS_MACOS)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/out)

# ffmpeg_wrapper_example
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

file(GLOB_RECURSE example_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${example_src})
add_executable(${project_name} ${example_src})
target_link_libraries(${project_name} ${common_name})

# Set this property in the same directory as a project() command call (e.g. in the top-level CMakeLists.txt file) to specify the default startup project for the corresponding solution file.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${project_name})
