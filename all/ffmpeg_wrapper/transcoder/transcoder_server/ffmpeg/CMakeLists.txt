# Created by liangxu on 2022/10/25.
#
# Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.20)

set(project_name ffmpeg)

project(${project_name})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Separate multiple Projects and put them into folders which are on top-level.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# How do I make CMake output into a 'bin' dir?
#   The correct variable to set is CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#   We use the following in our root CMakeLists.txt:
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WINDOWS)
  set(ffmpeg_config_dir ${CMAKE_CURRENT_SOURCE_DIR}/win)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/out/win_x64)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/x264/out/win_x64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/out/win_x86)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/x264/out/win_x86)
  endif()
  add_definitions(-DOS_WINDOWS)
elseif(MACOS)
  set(ffmpeg_config_dir ${CMAKE_CURRENT_SOURCE_DIR}/mac)
  set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/out/mac_x64)
  set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/x264/out/mac_x64)
endif()

include_directories(${ffmpeg_config_dir})
include_directories(${ffmpeg_root_dir}/include)
link_directories(${ffmpeg_root_dir}/lib)
link_directories(${x264_root_dir}/lib)

file(GLOB ffmpeg_src
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  )

file(GLOB platform_src ${ffmpeg_config_dir}/*.h)
list(APPEND ffmpeg_src ${platform_src})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ffmpeg_src})

add_executable(${project_name} ${ffmpeg_src})
if(WINDOWS)
  # ffmpeg libs
  target_link_libraries(${project_name} PRIVATE
    libavcodec
    libavdevice
    libavfilter
    libavformat
    libavutil
    libswresample
    libswscale
    libx264
    )
  if(EXISTS "${ffmpeg_root_dir}/lib/libpostproc.lib")
    target_link_libraries(${project_name} PRIVATE
      libpostproc
    )
  endif()
  # win32 libs
  target_link_libraries(${project_name} PRIVATE
    Strmiids
    Mfuuid
    Ws2_32
    Bcrypt
    Secur32
    Shlwapi
    Vfw32
    )
elseif(MACOS)
  # ffmpeg libs
  target_link_libraries(${project_name} PRIVATE
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    swresample
    swscale
    postproc
    x264
    )
  # 
  #target_link_libraries(${project_name} PRIVATE
  #  bzlib
  #  )
  # macos
  target_link_libraries(${project_name} PRIVATE
    "-framework appkit"
    "-framework avfoundation"
    "-framework coreimage"
    "-framework metal"
    "-framework CoreFoundation"
    "-framework Security"
    "-framework VideoDecodeAcceleration"
    "-framework AudioToolbox"
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework VideoToolbox"
    "-framework CoreAudio"
    "-framework OpenGL"
    )
endif()

if(WIN32)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${project_name})
  set_target_properties(${project_name} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-y -threads 4 -i \"D:/osknief/imsdk/hevc_8k60P_bilibili_1.mp4\" -c:0 copy -c:v:0 libx264 -c:a:0 aac -filter:v scale=1920:-1 \"$ENV{HOME}/Downloads/example_tmp.mp4\"")
else()
  
endif()
