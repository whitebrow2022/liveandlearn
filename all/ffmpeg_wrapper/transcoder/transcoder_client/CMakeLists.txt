cmake_minimum_required(VERSION 3.23)

set(project_name transcoder_client)

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

# ffmpeg
set(ffmpeg_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)
if(WIN32)
  set(ffmpeg_config_dir ${ffmpeg_src_dir}/win)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg/out/win_x64)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/x264/out/win_x64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg/out/win_x86)
    set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/x264/out/win_x86)
  endif()
  add_definitions(-DOS_WINDOWS)
elseif(APPLE)
  set(ffmpeg_config_dir ${ffmpeg_src_dir}/mac)
  set(ffmpeg_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ffmpeg/out/mac_x64)
  set(x264_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/x264/out/mac_x64)
endif()
message(STATUS "ffmpeg_config_dir is ${ffmpeg_config_dir}")
include_directories(${ffmpeg_config_dir})
include_directories(${ffmpeg_root_dir}/include)
link_directories(${ffmpeg_root_dir}/lib)
link_directories(${x264_root_dir}/lib)

# base log
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../transcoder_base/include)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network LinguistTools REQUIRED)

add_definitions(-DTRANSCODER_IMPLEMENTATION)

set(transcoder_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${transcoder_include_dir})
file(GLOB_RECURSE transcoder_public_header
  ${transcoder_include_dir}/*.h
  )
source_group(TREE ${transcoder_include_dir} PREFIX "Public API" FILES ${transcoder_public_header})

file(GLOB_RECURSE app_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX src FILES ${app_src})
file(GLOB_RECURSE app_ui ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX ui FILES ${app_ui})
file(GLOB_RECURSE app_res ${CMAKE_CURRENT_SOURCE_DIR}/*.ico ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX res FILES ${app_res})

#qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_library(${project_name} SHARED
  ${transcoder_public_header}
  ${app_src}
  ${app_ui}
  ${app_res}
  ${qm_files}
  ${project_name}.qrc
)

set_target_properties(${project_name} PROPERTIES 
  AUTOUIC ON
  AUTOMOC ON
  AUTORCC ON
)

target_link_libraries(${project_name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(${project_name} PRIVATE transcoder_base)

if(WIN32)
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
elseif(APPLE)
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
  set_target_properties(${project_name} PROPERTIES
    XCODE_ATTRIBUTE_INSTALL_PATH @executable_path/../Frameworks
    )
endif()
