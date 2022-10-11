# Created by %username% on %date%.
#
# Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.12)

set(project_name sharedlib_name)
if(IOS OR MACOS OR ANDROID)
  set(project_name sharedlib_name_common)  
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
option(SHAREDLIB_NAME_BUILD_SHARED_LIBS "Build ${project_name} as a shared library." ON)
option(SHAREDLIB_NAME_BUILD_STATIC_LIBS "Build ${project_name} as a static library." ON)
option(SHAREDLIB_NAME_BUILD_OBJECT_LIBS "Build ${project_name} as a object library." ON)
if(SHAREDLIB_NAME_BUILD_SHARED_LIBS)
  message(STATUS "======>> Build ${project_name} as a shared library.")
endif()
if(SHAREDLIB_NAME_BUILD_STATIC_LIBS)
  message(STATUS "======>> Build ${project_name} as a static library.")
endif()
if(SHAREDLIB_NAME_BUILD_OBJECT_LIBS)
  message(STATUS "======>> Build ${project_name} as a object library.")
endif()

add_subdirectory(third_party)

add_definitions(-DSHAREDLIB_NAME_IMPLEMENTATION)
if (WINDOWS)
  add_definitions(-DOS_WINDOWS)
elseif(ANDROID)
  add_definitions(-DOS_ANDROID)
elseif(MACOS)
  add_definitions(-DOS_MACOS)
elseif(IOS)
  add_definitions(-DOS_IOS)
endif()

set(sharedlib_name_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/xxx/include)

file(GLOB_RECURSE sharedlib_name_public_header
  ${sharedlib_name_include_dir}/*.h
  )

source_group(TREE ${sharedlib_name_include_dir} PREFIX "Public API" FILES ${sharedlib_name_public_header})

file(GLOB_RECURSE sharedlib_name_src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  )

# Removes items from the list that match the mode's pattern.
# Removes the content of 'src/platform' folder.
list(FILTER sharedlib_name_src EXCLUDE REGEX ".*platform(.)*")

#if(ANDROID)
#  file(GLOB_RECURSE jni_src
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cc
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.h
#    )
#  list(APPEND sharedlib_name_src ${jni_src})
#endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sharedlib_name_src})

#add_library(${project_name} SHARED ${sharedlib_name_src} ${sharedlib_name_public_header})
#target_link_libraries(${project_name} xxx yyy)

if(MACOS)
  set(build_os_name "macos")
else(IOS)
  set(build_os_name "ios")
endif()

# build different libs
if (SHAREDLIB_NAME_BUILD_SHARED_LIBS)
  set(shared_lib ${project_name}_lib)
  add_library(${shared_lib} SHARED ${sharedlib_name_src} ${sharedlib_name_public_header})
  set_target_properties(${shared_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${SHAREDLIB_NAME_BUILD_SHARED_LIBS}
    )
  target_include_directories(${shared_lib} PUBLIC 
    $<BUILD_INTERFACE:${sharedlib_name_include_dir}>
    )
  target_compile_definitions(${shared_lib} PUBLIC SHAREDLIB_NAME_COMPONENT_BUILD)
  if(IOS OR MACOS)
    set_target_properties(${shared_lib} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.example.sharedlib-name-common
      VALID_ARCHS[variant=Release] "arm64 armv7"
      VALID_ARCHS[variant=Debug] "arm64 armv7 x86_64"
    )
  endif()
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(sharedlib_name_shared_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/sharedlib_name/lib/$(CONFIGURATION)")
      if(MACOS)
        set(sharedlib_name_shared_lib "${sharedlib_name_shared_lib_path}/sharedlib_name_common.framework/Versions/A/sharedlib_name_common")
      else()
        set(sharedlib_name_shared_lib "${sharedlib_name_shared_lib_path}/sharedlib_name_common.framework/sharedlib_name_common")
      endif()
      if(USE_SHARED_LIBS)
        message(STATUS "======>> use shared libs ${USE_SHARED_LIBS}")
        set(sharedlib_name_common_lib "${sharedlib_name_shared_lib}")
        set(sharedlib_name_common_lib_path "${sharedlib_name_shared_lib_path}")
      endif()
    endif()
  endif()
endif()
if (SHAREDLIB_NAME_BUILD_STATIC_LIBS)
  set(static_lib ${project_name}_static)
  add_library(${static_lib} STATIC ${sharedlib_name_src} ${sharedlib_name_public_header})
  # avoid name clashes on windows as the shared import lib is also named jsoncpp.lib
  if(NOT DEFINED static_suffix AND SHAREDLIB_NAME_BUILD_SHARED_LIBS)
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
    $<BUILD_INTERFACE:${sharedlib_name_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(sharedlib_name_static_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/sharedlib_name/lib/$(CONFIGURATION)")
      set(sharedlib_name_static_lib "${sharedlib_name_static_lib_path}/libsharedlib_name_common.a")
      if(USE_STATIC_LIBS)
        message(STATUS "======>> use static libs ${USE_STATIC_LIBS}")
        set(sharedlib_name_common_lib "${sharedlib_name_static_lib}")
        set(sharedlib_name_common_lib_path "")
      endif()
    endif()
  endif()
endif()
if (SHAREDLIB_NAME_BUILD_OBJECT_LIBS)
  set(object_lib ${project_name}_object)
  add_library(${object_lib} OBJECT ${sharedlib_name_src} ${sharedlib_name_public_header})
  set_target_properties(${object_lib} PROPERTIES
    OUTPUT_NAME ${project_name}
    POSITION_INDEPENDENT_CODE ${SHAREDLIB_NAME_BUILD_SHARED_LIBS}
    )
  target_include_directories(${object_lib} PUBLIC 
    $<BUILD_INTERFACE:${sharedlib_name_include_dir}>
    )
  if(GENERATE_XCCONFIG)
    if(MACOS OR IOS)
      set(sharedlib_name_object_lib_path "${CMAKE_CURRENT_SOURCE_DIR}/build/${build_os_name}/apps/sharedlib_name/sharedlib_name_common.build/$(CONFIGURATION)/sharedlib_name_common_object.build")
      set(sharedlib_name_object_lib "${sharedlib_name_object_lib_path}/libsharedlib_name_common_object.a")
      if(USE_OBJECT_LIBS)
        message(STATUS "======>> use object libs ${USE_OBJECT_LIBS}")
        set(sharedlib_name_common_lib "${sharedlib_name_object_lib}")
        set(sharedlib_name_common_lib_path "")
      endif()
    endif()
  endif()
endif()

if(GENERATE_XCCONFIG)
  if(MACOS OR IOS)
    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/${build_os_name}/SharedlibName/SharedlibName/SharedlibNameCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for SharedlibName ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_INCLUDE_PATH = ${CMAKE_CURRENT_SOURCE_DIR}/include\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_SHARED_LIB_PATH = ${sharedlib_name_shared_lib_path}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_SHARED_LIB = ${sharedlib_name_shared_lib}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_STATIC_LIB_PATH = ${sharedlib_name_static_lib_path}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_STATIC_LIB = ${sharedlib_name_static_lib}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_OBJECT_LIB_PATH = ${sharedlib_name_object_lib_path}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_OBJECT_LIB = ${sharedlib_name_object_lib}\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_LIB = ${sharedlib_name_common_lib}\n")
    file(APPEND ${xcconfig_path} "\n")

    set(xcconfig_path "${CMAKE_CURRENT_SOURCE_DIR}/apps/sharedlib_name_app/${build_os_name}/SharedlibNameApp/SharedlibNameApp/SharedlibNameCommon.xcconfig")
    message(STATUS "generate ${build_os_name} xcconfig for SharedlibNameApp ...")
    file(WRITE ${xcconfig_path} "// Configuration settings file format documentation can be found at:\n")
    file(APPEND ${xcconfig_path} "// https://help.apple.com/xcode/#/dev745c5c974\n")
    file(APPEND ${xcconfig_path} "\n\n")
    file(APPEND ${xcconfig_path} "SHAREDLIB_NAME_LIB_PATH = ${sharedlib_name_common_lib_path}\n")
    file(APPEND ${xcconfig_path} "\n")
  endif()
  #if(IOS)
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/ios/SharedlibName/SharedlibName/SharedlibNameCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/sharedlib_name/lib/$(CONFIGURATION)")
  #  generate_xcconfig("${CMAKE_CURRENT_SOURCE_DIR}/apps/sharedlib_name_app/ios/SharedlibNameApp/SharedlibNameApp/SharedlibNameCommon.xcconfig" "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/apps/sharedlib_name/lib/$(CONFIGURATION)")
  #endif()
endif()
