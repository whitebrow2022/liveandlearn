#!/bin/bash
#
# Generate 'macOS' project for FfmpegWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_path="${HOME}/Qt/5.15.2/clang_64"
qt_osx_dir="$bash_dir/build/macos_qt/apps/ffmpeg_wrapper_app"
cmake -S "$bash_dir/apps/ffmpeg_wrapper_app/qt/FfmpegWrapperApp" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DMACOS:BOOL=ON -DCMAKE_PREFIX_PATH=${qt_osx_path} -DFFMPEG_WRAPPER_BUILD_SHARED_LIBS=ON -DFFMPEG_WRAPPER_BUILD_STATIC_LIBS=OFF -DFFMPEG_WRAPPER_BUILD_OBJECT_LIBS=OFF -DUSE_FFMPEG_WRAPPER_SHARED_LIBS=ON -DUSE_FFMPEG_WRAPPER_STATIC_LIBS=OFF -DUSE_FFMPEG_WRAPPER_OBJECT_LIBS=OFF

app_xcodeproj="$bash_dir/QtApp.xcodeproj"
if ! [ -h "$app_xcodeproj" ]; then
  ln -s "$qt_osx_dir/ffmpeg_wrapper_app.xcodeproj" "$app_xcodeproj"
fi

echo "success!"