# Created by liangxu on 2022/10/25.
#
# Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.20)

set(project_name ffmpeg_wrapper)
if(IOS OR MACOS OR ANDROID)
  set(project_name ffmpeg_wrapper_common)  
endif()

project(${project_name})

set(CMAKE_CXX_STANDARD 20)
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

#
option(FFMPEG_WRAPPER_BUILD_SHARED_LIBS "Build ${project_name} as a shared library." ON)
option(FFMPEG_WRAPPER_BUILD_STATIC_LIBS "Build ${project_name} as a static library." ON)
option(FFMPEG_WRAPPER_BUILD_OBJECT_LIBS "Build ${project_name} as a object library." ON)
if(FFMPEG_WRAPPER_BUILD_SHARED_LIBS)
  message(STATUS "======>> Build ${project_name} as a shared library.")
endif()
if(FFMPEG_WRAPPER_BUILD_STATIC_LIBS)
  message(STATUS "======>> Build ${project_name} as a static library.")
endif()
if(FFMPEG_WRAPPER_BUILD_OBJECT_LIBS)
  message(STATUS "======>> Build ${project_name} as a object library.")
endif()

add_subdirectory(third_party)

add_definitions(-DFFMPEG_WRAPPER_IMPLEMENTATION)

if(WINDOWS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/out/win_x64)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/x264/out/win_x64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/out/win_x86)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/x264/out/win_x86)
  endif()
  add_definitions(-DOS_WINDOWS)
elseif(MACOS)
  set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/out/mac_x64)
  set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party/x264/out/mac_x64)
elseif(ANDROID)
  add_definitions(-DOS_ANDROID)
elseif(IOS)
  add_definitions(-DOS_IOS)
endif()

include_directories(${ffmpeg_root_dir}/include)
link_directories(${ffmpeg_root_dir}/lib)
link_directories(${x264_root_dir}/lib)

set(ffmpeg_wrapper_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/xxx/include)

file(GLOB_RECURSE ffmpeg_wrapper_public_header
  ${ffmpeg_wrapper_include_dir}/*.h
  )

source_group(TREE ${ffmpeg_wrapper_include_dir} PREFIX "Public API" FILES ${ffmpeg_wrapper_public_header})

file(GLOB_RECURSE ffmpeg_wrapper_src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  )

# Removes items from the list that match the mode's pattern.
# Removes the content of 'src/platform' folder.
list(FILTER ffmpeg_wrapper_src EXCLUDE REGEX ".*platform(.)*")

#if(ANDROID)
#  file(GLOB_RECURSE jni_src
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cc
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.h
#    )
#  list(APPEND ffmpeg_wrapper_src ${jni_src})
#endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ffmpeg_wrapper_src})

#add_library(${project_name} SHARED ${ffmpeg_wrapper_src} ${ffmpeg_wrapper_public_header})
#target_link_libraries(${project_name} xxx yyy)

if(MACOS)
  set(build_os_name "macos")
else(IOS)
  set(build_os_name "ios")
endif()

# build different libs
if (FFMPEG_WRAPPER_BUILD_SHARED_LIBS)
  set(shared_lib ${project_name}_lib)
  add_library(${shared_lib} SHARED ${ffmpeg_wrapper_src} ${ffmpeg_wrapper_public_header})
  if(WINDOWS)
    # ffmpeg libs
    target_link_libraries(${shared_lib} PRIVATE
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
      target_link_libraries(${shared_lib} PRIVATE
        libpostproc
      )
    endif()
    # win32 libs
    target_link_libraries(${shared_lib} PRIVATE
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
    target_link_libraries(${shared_lib} PRIVATE
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
    #target_link_libraries(${shared_lib} PRIVATE
    #  bzlib
    #  )
    # macos
    target_link_libraries(${shared_lib} PRIVATE
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
  set_target_properties(${shared_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${FFMPEG_WRAPPER_BUILD_SHARED_LIBS}
    )
  target_include_directories(${shared_lib} PUBLIC 
    $<BUILD_INTERFACE:${ffmpeg_wrapper_include_dir}>
    )
  target_compile_definitions(${shared_lib} PUBLIC FFMPEG_WRAPPER_COMPONENT_BUILD)
  if(IOS OR MACOS)
    set_target_properties(${shared_lib} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.example.ffmpeg-wrapper-common
      VALID_ARCHS[variant=Release] "arm64 armv7"
      VALID_ARCHS[variant=Debug] "arm64 armv7 x86_64"
    )
  endif()
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(ffmpeg_wrapper_shared_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/ffmpeg_wrapper/lib/$(CONFIGURATION)")
      if(MACOS)
        set(ffmpeg_wrapper_shared_lib "${ffmpeg_wrapper_shared_lib_path}/ffmpeg_wrapper_common.framework/Versions/A/ffmpeg_wrapper_common")
      else()
        set(ffmpeg_wrapper_shared_lib "${ffmpeg_wrapper_shared_lib_path}/ffmpeg_wrapper_common.framework/ffmpeg_wrapper_common")
      endif()
      if(USE_SHARED_LIBS)
        message(STATUS "======>> use shared libs ${USE_SHARED_LIBS}")
        set(ffmpeg_wrapper_common_lib "${ffmpeg_wrapper_shared_lib}")
        set(ffmpeg_wrapper_common_lib_path "${ffmpeg_wrapper_shared_lib_path}")
      endif()
    endif()
  endif()
endif()
if (FFMPEG_WRAPPER_BUILD_STATIC_LIBS)
  set(static_lib ${project_name}_static)
  add_library(${static_lib} STATIC ${ffmpeg_wrapper_src} ${ffmpeg_wrapper_public_header})
  # avoid name clashes on windows as the shared import lib is also named jsoncpp.lib
  if(NOT DEFINED static_suffix AND FFMPEG_WRAPPER_BUILD_SHARED_LIBS)
    if(MSVC OR ANDROID)
      set(static_suffix "_static")
    else()
      set(static_suffix "")
    endif()
  endif()
  set_target_properties(${static_lib} PROPERTIES
    OUTPUT_NAME ${project_name}${static_suffix}
    )
  target_include_directories(${static_lib} PUBLIC 
    $<BUILD_INTERFACE:${ffmpeg_wrapper_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(ffmpeg_wrapper_static_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/ffmpeg_wrapper/lib/$(CONFIGURATION)")
      set(ffmpeg_wrapper_static_lib "${ffmpeg_wrapper_static_lib_path}/libffmpeg_wrapper_common.a")
      if(USE_STATIC_LIBS)
        message(STATUS "======>> use static libs ${USE_STATIC_LIBS}")
        set(ffmpeg_wrapper_common_lib "${ffmpeg_wrapper_static_lib}")
        set(ffmpeg_wrapper_common_lib_path "")
      endif()
    endif()
  endif()
endif()
if (FFMPEG_WRAPPER_BUILD_OBJECT_LIBS)
  set(object_lib ${project_name}_object)
  add_library(${object_lib} OBJECT ${ffmpeg_wrapper_src} ${ffmpeg_wrapper_public_header})
  set_target_properties(${object_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${FFMPEG_WRAPPER_BUILD_SHARED_LIBS}
    )
  target_include_directories(${object_lib} PUBLIC 
    $<BUILD_INTERFACE:${ffmpeg_wrapper_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(ffmpeg_wrapper_object_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/ffmpeg_wrapper/ffmpeg_wrapper_common.build/$(CONFIGURATION)/ffmpeg_wrapper_common_object.build")
      set(ffmpeg_wrapper_object_lib "${ffmpeg_wrapper_object_lib_path}/libffmpeg_wrapper_common_object.a")
      if(USE_OBJECT_LIBS)
        message(STATUS "======>> use object libs ${USE_OBJECT_LIBS}")
        set(ffmpeg_wrapper_common_lib "${ffmpeg_wrapper_object_lib}")
        set(ffmpeg_wrapper_common_lib_path "")
      endif()
    endif()
  endif()
endif()

if(GENERATE_XCCONFIG)
  if(MACOS OR IOS)
    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/${build_os_name}/FfmpegWrapper/FfmpegWrapper/FfmpegWrapperCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for FfmpegWrapper ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_INCLUDE_PATH = ${CMAKE_CURRENT_SOURCE_DIR}/include\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_SHARED_LIB_PATH = ${ffmpeg_wrapper_shared_lib_path}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_SHARED_LIB = ${ffmpeg_wrapper_shared_lib}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_STATIC_LIB_PATH = ${ffmpeg_wrapper_static_lib_path}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_STATIC_LIB = ${ffmpeg_wrapper_static_lib}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_OBJECT_LIB_PATH = ${ffmpeg_wrapper_object_lib_path}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_OBJECT_LIB = ${ffmpeg_wrapper_object_lib}\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_LIB = ${ffmpeg_wrapper_common_lib}\n")
    file(APPEND ${xcconfig_path} "\n")

    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/apps/ffmpeg_wrapper_app/${build_os_name}/FfmpegWrapperApp/FfmpegWrapperApp/FfmpegWrapperCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for FfmpegWrapperApp ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "FFMPEG_WRAPPER_LIB_PATH = ${ffmpeg_wrapper_common_lib_path}\n")
    file(APPEND ${xcconfig_path} "\n")
  endif()
  #if(IOS)
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/ios/FfmpegWrapper/FfmpegWrapper/FfmpegWrapperCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/ffmpeg_wrapper/lib/$(CONFIGURATION)")
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/apps/ffmpeg_wrapper_app/ios/FfmpegWrapperApp/FfmpegWrapperApp/FfmpegWrapperCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/ffmpeg_wrapper/lib/$(CONFIGURATION)")
  #endif()
endif()
