cmake_minimum_required(VERSION 3.21)

project(fun_cef)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Separate multiple Projects and put them into folders which are on top-level.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/cef_config.cmake")

if(WIN32 AND USE_SANDBOX)
  # Check if the current msvc version is compatible with the cef_sandbox.lib
  # static library. We require VS2015 or newer.
  if(MSVC_VERSION LESS 1900)
    message(WARNING "cef sandbox is not compatible with the current MSVC version (${MSVC_VERSION})")
    set(USE_SANDBOX OFF)
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

if(APPLE)
  # command line arguments
  set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
endif()

# -Wno-everything
# [-Werror,-Woverriding-t-option]
add_compile_options("$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-overriding-t-option>")

add_subdirectory(${cef_sdk_dir} cef_prebuilt)
set_target_properties(cef_gtest PROPERTIES FOLDER "cef")
set_target_properties(cefclient PROPERTIES FOLDER "cef")
set_target_properties(cefsimple PROPERTIES FOLDER "cef")
set_target_properties(ceftests PROPERTIES FOLDER "cef")
set_target_properties(libcef_dll_wrapper PROPERTIES FOLDER "cef")
set_target_properties(cefsimple PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--url=file:///\"${CMAKE_CURRENT_SOURCE_DIR}/../res/index.html\"")

# Display configuration settings.
PRINT_CEF_CONFIG()

# How do I make CMake output into a 'bin' dir?
#   The correct variable to set is CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#   We use the following in our root CMakeLists.txt:
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(cef_demo)
