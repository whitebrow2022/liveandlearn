cmake_minimum_required(VERSION 3.19)

set(project_name sharedlib_name_app)

project(${project_name} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/sharedlib_name.cmake)

message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ${CMAKE_CURRENT_BINARY_DIR}/out)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)

set(TS_FILES sharedlib_name_app_zh.ts)

file(GLOB_RECURSE app_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${app_src})
file(GLOB_RECURSE app_ui ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${app_ui})
file(GLOB_RECURSE app_res ${CMAKE_CURRENT_SOURCE_DIR}/*.ico ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${app_res})

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(${project_name}
  ${app_src}
  ${app_ui}
  ${app_res}
  ${QM_FILES}
  ${project_name}.qrc
)
set_property(TARGET ${project_name} PROPERTY AUTOUIC ON)
set_property(TARGET ${project_name} PROPERTY AUTOMOC ON)
set_property(TARGET ${project_name} PROPERTY AUTORCC ON)

if(WIN32)
  find_package(QT NAMES Qt6 Qt5 COMPONENTS WinExtras REQUIRED)
  find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WinExtras REQUIRED)
  set(EXTRAS_LIBS Qt${QT_VERSION_MAJOR}::WinExtras)
endif()
target_link_libraries(${project_name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${EXTRAS_LIBS} ${common_name})

if(MSVC)
  list(APPEND qt_dll_lists "Qt5Core" "Qt5Gui" "Qt5Widgets" "Qt5WinExtras")
  foreach(qt_dll_name IN LISTS qt_dll_lists)
    file(COPY ${QT_DLL_DIR}/${qt_dll_name}d.dll DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug")
  endforeach()
  foreach(qt_dll_name IN LISTS qt_dll_lists)
    file(COPY ${QT_DLL_DIR}/${qt_dll_name}.dll DESTINATION "${CMAKE_BINARY_DIR}/bin/Release")
  endforeach()
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${project_name})
endif()
