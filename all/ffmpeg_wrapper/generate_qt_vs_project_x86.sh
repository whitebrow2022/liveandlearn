#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo Generate ffmpeg_wrapper x86 qt app
qt_msvc_path="C:/Qt/Qt5.15.2/5.15.2/msvc2019"
vs_qt_dir=${bash_dir}/build/windows_qt/apps/ffmpeg_wrapper_app_x86
cmake -S "${bash_dir}/apps/ffmpeg_wrapper_app/qt/FfmpegWrapperApp" -B "${vs_qt_dir}" -G "Visual Studio 17 2022" -A Win32 -T v143 -DWINDOWS:BOOL=ON -DCMAKE_PREFIX_PATH=${qt_msvc_path} -DQT_MSVC_DIR="${qt_msvc_path}" -DFFMPEG_WRAPPER_BUILD_SHARED_LIBS=ON -DFFMPEG_WRAPPER_BUILD_STATIC_LIBS=OFF -DFFMPEG_WRAPPER_BUILD_OBJECT_LIBS=OFF -DUSE_FFMPEG_WRAPPER_SHARED_LIBS=ON -DUSE_FFMPEG_WRAPPER_STATIC_LIBS=OFF -DUSE_FFMPEG_WRAPPER_OBJECT_LIBS=OFF

echo "..."
