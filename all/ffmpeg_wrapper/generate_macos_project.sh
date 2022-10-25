#!/bin/bash
#
# Generate 'macOS' project for FfmpegWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate macos project ..."

lib_dir="$bash_dir/build/macos/apps/ffmpeg_wrapper"
#cmake -S "$bash_dir" -B "$lib_dir" -G "Xcode" -DMACOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_SHARED_LIBS:BOOL=ON
cmake -S "$bash_dir" -B "$lib_dir" -G "Xcode" -DMACOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_STATIC_LIBS:BOOL=ON
#cmake -S "$bash_dir" -B "$lib_dir" -G "Xcode" -DMACOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_OBJECT_LIBS:BOOL=ON

app_xcworkspace="$bash_dir/MacApp.xcworkspace"
if ! [ -h "$app_xcworkspace" ]; then
  ln -s "$bash_dir/apps/ffmpeg_wrapper_app/macos/FfmpegWrapperApp.xcworkspace" "$app_xcworkspace"
fi

echo "success!"
