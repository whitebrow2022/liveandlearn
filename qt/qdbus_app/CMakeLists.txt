# Created by liangxu on 2022/11/13.
#
# Copyright (c) 2022 The Qdbus Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# 

cmake_minimum_required(VERSION 3.23)

set(project_name qdbus)

project(${project_name})

set(CMAKE_CXX_STANDARD 17)
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

# generate dbus interface
#find_package(Qt5 CONFIG REQUIRED Core DBus)
#qt5_generate_dbus_interface(car.h
#${CMAKE_CURRENT_SOURCE_DIR}/car.xml
#  OPTIONS -A
#  )
#qt5_add_dbus_interface(dbus_interface_srcs
#  ${CMAKE_CURRENT_SOURCE_DIR}/org.example.dbus.xml
#  dbus_interface
#  )
#qt5_add_dbus_adaptor(dbus_interface_srcs
#  ${CMAKE_CURRENT_SOURCE_DIR}/car.xml
#  )
#message(STATUS "dbus interface srcs: ${dbus_interface_srcs}")

add_subdirectory(qdbus_client/qdbus_client_app)
add_subdirectory(qdbus_server/qdbus_server_app)
