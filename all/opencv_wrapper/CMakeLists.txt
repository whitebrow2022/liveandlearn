# Created by liangxu on 2022/10/21.
#
# Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.12)

set(project_name opencv_wrapper)
if(IOS OR MACOS OR ANDROID)
  set(project_name opencv_wrapper_common)  
endif()

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

#
option(OPENCV_WRAPPER_BUILD_SHARED_LIBS "Build ${project_name} as a shared library." ON)
option(OPENCV_WRAPPER_BUILD_STATIC_LIBS "Build ${project_name} as a static library." OFF)
option(OPENCV_WRAPPER_BUILD_OBJECT_LIBS "Build ${project_name} as a object library." OFF)
if(OPENCV_WRAPPER_BUILD_SHARED_LIBS)
  message(STATUS "======>> Build ${project_name} as a shared library.")
endif()
if(OPENCV_WRAPPER_BUILD_STATIC_LIBS)
  message(STATUS "======>> Build ${project_name} as a static library.")
endif()
if(OPENCV_WRAPPER_BUILD_OBJECT_LIBS)
  message(STATUS "======>> Build ${project_name} as a object library.")
endif()

# ffmpeg
if(OPENCV_FFMPEG_USE_FIND_PACKAGE)
  set(HAVE_FFMPEG ON)
  set(OPENCV_FFMPEG_SKIP_BUILD_CHECK ON)
  set(FFMPEG_libavcodec_FOUND 1)
  set(FFMPEG_libavformat_FOUND 1)
  set(FFMPEG_libavutil_FOUND 1)
  set(FFMPEG_libswscale_FOUND 1)
  set(FFMPEG_libavresample_FOUND 1)
  set(FFMPEG_libavcodec_VERSION 58.134.100)
  set(FFMPEG_libavformat_VERSION 58.76.100)
  set(FFMPEG_libavutil_VERSION 56.70.100)
  set(FFMPEG_libswscale_VERSION 5.9.100)
  set(FFMPEG_libavresample_VERSION 4.0.0)

  set(ffmpeg_install_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/out/win_x64")
  include_directories(${ffmpeg_install_dir}/include)
  link_directories(${ffmpeg_install_dir}/lib)
endif()
if (OPENCV_FFMPEG_USE_FIND_PACKAGE AND NOT HAVE_FFMPEG)
  message(FATAL_ERROR "ffmpeg not have")
endif()

add_subdirectory(third_party)

add_definitions(-DOPENCV_WRAPPER_IMPLEMENTATION)
if (WINDOWS)
  add_definitions(-DOS_WINDOWS)
elseif(ANDROID)
  add_definitions(-DOS_ANDROID)
elseif(MACOS)
  add_definitions(-DOS_MACOS)
elseif(IOS)
  add_definitions(-DOS_IOS)
endif()

set(opencv_wrapper_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/xxx/include)

file(GLOB_RECURSE opencv_wrapper_public_header
  ${opencv_wrapper_include_dir}/*.h
  )

source_group(TREE ${opencv_wrapper_include_dir} PREFIX "Public API" FILES ${opencv_wrapper_public_header})

file(GLOB_RECURSE opencv_wrapper_src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  )

# Removes items from the list that match the mode's pattern.
# Removes the content of 'src/platform' folder.
list(FILTER opencv_wrapper_src EXCLUDE REGEX ".*platform(.)*")

#if(ANDROID)
#  file(GLOB_RECURSE jni_src
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cc
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.h
#    )
#  list(APPEND opencv_wrapper_src ${jni_src})
#endif()

# opencv
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/core/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/calib3d/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/dnn/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/features2d/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/video/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/videoio/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party/src/opencv/modules/imgcodecs/include")
include_directories("${CMAKE_BINARY_DIR}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${opencv_wrapper_src})

#add_library(${project_name} SHARED ${opencv_wrapper_src} ${opencv_wrapper_public_header})
#target_link_libraries(${project_name} xxx yyy)

if(MACOS)
  set(build_os_name "macos")
else(IOS)
  set(build_os_name "ios")
endif()

# build different libs
if (OPENCV_WRAPPER_BUILD_SHARED_LIBS)
  set(shared_lib ${project_name}_lib)
  add_library(${shared_lib} SHARED ${opencv_wrapper_src} ${opencv_wrapper_public_header})
  target_link_libraries(${shared_lib} PRIVATE
    opencv_calib3d
    opencv_core
    opencv_features2d
    opencv_flann
    opencv_highgui
    opencv_imgcodecs
    opencv_imgproc
    opencv_ml
    opencv_objdetect
    opencv_photo
    opencv_stitching
    opencv_video
    opencv_videoio
  )
  if(HAVE_FFMPEG AND WIN32)
    # ffmpeg libs
    target_link_libraries(${shared_lib} PRIVATE
      libavcodec
      libavdevice
      libavfilter
      libavformat
      libavutil
      libswresample
      libswscale
      )
    # win32 libs
    target_link_libraries(${shared_lib} PRIVATE
      Strmiids
      Mfuuid
      Ws2_32
      Bcrypt
      Secur32
      )
  endif()
  set_target_properties(${shared_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${OPENCV_WRAPPER_BUILD_SHARED_LIBS}
    )
  target_include_directories(${shared_lib} PUBLIC 
    $<BUILD_INTERFACE:${opencv_wrapper_include_dir}>
    )
  target_compile_definitions(${shared_lib} PUBLIC OPENCV_WRAPPER_COMPONENT_BUILD)
  if(IOS OR MACOS)
    set_target_properties(${shared_lib} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.example.opencv-wrapper-common
      VALID_ARCHS[variant=Release] "arm64 armv7"
      VALID_ARCHS[variant=Debug] "arm64 armv7 x86_64"
    )
  endif()
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(opencv_wrapper_shared_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/opencv_wrapper/lib/$(CONFIGURATION)")
      if(MACOS)
        set(opencv_wrapper_shared_lib "${opencv_wrapper_shared_lib_path}/opencv_wrapper_common.framework/Versions/A/opencv_wrapper_common")
      else()
        set(opencv_wrapper_shared_lib "${opencv_wrapper_shared_lib_path}/opencv_wrapper_common.framework/opencv_wrapper_common")
      endif()
      if(USE_SHARED_LIBS)
        message(STATUS "======>> use shared libs ${USE_SHARED_LIBS}")
        set(opencv_wrapper_common_lib "${opencv_wrapper_shared_lib}")
        set(opencv_wrapper_common_lib_path "${opencv_wrapper_shared_lib_path}")
      endif()
    endif()
  endif()
endif()
if (OPENCV_WRAPPER_BUILD_STATIC_LIBS)
  set(static_lib ${project_name}_static)
  add_library(${static_lib} STATIC ${opencv_wrapper_src} ${opencv_wrapper_public_header})
  # avoid name clashes on windows as the shared import lib is also named jsoncpp.lib
  if(NOT DEFINED static_suffix AND OPENCV_WRAPPER_BUILD_SHARED_LIBS)
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
    $<BUILD_INTERFACE:${opencv_wrapper_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(opencv_wrapper_static_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/opencv_wrapper/lib/$(CONFIGURATION)")
      set(opencv_wrapper_static_lib "${opencv_wrapper_static_lib_path}/libopencv_wrapper_common.a")
      if(USE_STATIC_LIBS)
        message(STATUS "======>> use static libs ${USE_STATIC_LIBS}")
        set(opencv_wrapper_common_lib "${opencv_wrapper_static_lib}")
        set(opencv_wrapper_common_lib_path "")
      endif()
    endif()
  endif()
endif()
if (OPENCV_WRAPPER_BUILD_OBJECT_LIBS)
  set(object_lib ${project_name}_object)
  add_library(${object_lib} OBJECT ${opencv_wrapper_src} ${opencv_wrapper_public_header})
  set_target_properties(${object_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${OPENCV_WRAPPER_BUILD_SHARED_LIBS}
    )
  target_include_directories(${object_lib} PUBLIC 
    $<BUILD_INTERFACE:${opencv_wrapper_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(opencv_wrapper_object_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/opencv_wrapper/opencv_wrapper_common.build/$(CONFIGURATION)/opencv_wrapper_common_object.build")
      set(opencv_wrapper_object_lib "${opencv_wrapper_object_lib_path}/libopencv_wrapper_common_object.a")
      if(USE_OBJECT_LIBS)
        message(STATUS "======>> use object libs ${USE_OBJECT_LIBS}")
        set(opencv_wrapper_common_lib "${opencv_wrapper_object_lib}")
        set(opencv_wrapper_common_lib_path "")
      endif()
    endif()
  endif()
endif()

if(GENERATE_XCCONFIG)
  if(MACOS OR IOS)
    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/${build_os_name}/OpencvWrapper/OpencvWrapper/OpencvWrapperCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for OpencvWrapper ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_INCLUDE_PATH = ${CMAKE_CURRENT_SOURCE_DIR}/include\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_SHARED_LIB_PATH = ${opencv_wrapper_shared_lib_path}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_SHARED_LIB = ${opencv_wrapper_shared_lib}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_STATIC_LIB_PATH = ${opencv_wrapper_static_lib_path}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_STATIC_LIB = ${opencv_wrapper_static_lib}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_OBJECT_LIB_PATH = ${opencv_wrapper_object_lib_path}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_OBJECT_LIB = ${opencv_wrapper_object_lib}\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_LIB = ${opencv_wrapper_common_lib}\n")
    file(APPEND ${xcconfig_path} "\n")

    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/apps/opencv_wrapper_app/${build_os_name}/OpencvWrapperApp/OpencvWrapperApp/OpencvWrapperCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for OpencvWrapperApp ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "OPENCV_WRAPPER_LIB_PATH = ${opencv_wrapper_common_lib_path}\n")
    file(APPEND ${xcconfig_path} "\n")
  endif()
  #if(IOS)
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/ios/OpencvWrapper/OpencvWrapper/OpencvWrapperCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/opencv_wrapper/lib/$(CONFIGURATION)")
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/apps/opencv_wrapper_app/ios/OpencvWrapperApp/OpencvWrapperApp/OpencvWrapperCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/opencv_wrapper/lib/$(CONFIGURATION)")
  #endif()
endif()
