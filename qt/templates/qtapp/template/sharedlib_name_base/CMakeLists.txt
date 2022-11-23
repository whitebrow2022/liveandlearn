cmake_minimum_required(VERSION 3.23)

set(project_name sharedlib_name_base)

project(${project_name} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_CXX_STANDARD 17)
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

message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")

# base export
add_definitions(-DSHAREDLIB_NAME_BASE_IMPLEMENTATION)
if(WIN32)
  add_definitions(-DOS_WINDOWS)
elseif(APPLE)
  add_definitions(-DOS_MACOS)
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

set(sharedlib_name_base_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${sharedlib_name_base_include_dir})
file(GLOB_RECURSE sharedlib_name_base_public_header
  ${sharedlib_name_base_include_dir}/*.h
  )
source_group(TREE ${sharedlib_name_base_include_dir} PREFIX "Public API" FILES ${sharedlib_name_base_public_header})

set(sharedlib_name_base_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE app_src ${sharedlib_name_base_src_dir}/*.h ${sharedlib_name_base_src_dir}/*.cc)
source_group(TREE ${sharedlib_name_base_src_dir} PREFIX src FILES ${app_src})
file(GLOB_RECURSE app_ui ${sharedlib_name_base_src_dir}/*.ui)
source_group(TREE ${sharedlib_name_base_src_dir} PREFIX ui FILES ${app_ui})
file(GLOB_RECURSE app_res ${sharedlib_name_base_src_dir}/*.ico ${sharedlib_name_base_src_dir}/*.rc)
source_group(TREE ${sharedlib_name_base_src_dir} PREFIX res FILES ${app_res})

#qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_library(${project_name} SHARED
  ${sharedlib_name_base_public_header}
  ${app_src}
  ${app_ui}
  ${app_res}
  ${qm_files}
)

set_target_properties(${project_name} PROPERTIES 
  AUTOUIC ON
  AUTOMOC ON
  AUTORCC ON
)

target_link_libraries(${project_name} PRIVATE Qt${QT_VERSION_MAJOR}::Core)

if(WIN32)
elseif(APPLE)
  set_target_properties(${project_name} PROPERTIES
    XCODE_ATTRIBUTE_INSTALL_PATH @executable_path/../Frameworks
    )
endif()
