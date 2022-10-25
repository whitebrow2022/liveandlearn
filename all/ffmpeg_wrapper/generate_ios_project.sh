#!/bin/bash
#
# Generate 'iOS' project for FfmpegWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate ios project ..."

ios_proj_dir="$bash_dir/build/ios/apps/ffmpeg_wrapper"
#cmake -S "$bash_dir" -B "$ios_proj_dir" -G "Xcode" -DCMAKE_SYSTEM_NAME=iOS "-DCMAKE_OSX_ARCHITECTURES=armv7;armv7s;arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.0 -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO -DIOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_SHARED_LIBS:BOOL=ON
cmake -S "$bash_dir" -B "$ios_proj_dir" -G "Xcode" -DCMAKE_SYSTEM_NAME=iOS "-DCMAKE_OSX_ARCHITECTURES=armv7;armv7s;arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.0 -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO -DIOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_STATIC_LIBS:BOOL=ON
#cmake -S "$bash_dir" -B "$ios_proj_dir" -G "Xcode" -DCMAKE_SYSTEM_NAME=iOS "-DCMAKE_OSX_ARCHITECTURES=armv7;armv7s;arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.0 -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO -DIOS:BOOL=ON -DGENERATE_XCCONFIG:BOOL=ON -DUSE_OBJECT_LIBS:BOOL=ON

app_xcworkspace="$bash_dir/IosApp.xcworkspace"
if ! [ -h "$app_xcworkspace" ]; then
  ln -s "$bash_dir/apps/ffmpeg_wrapper_app/ios/FfmpegWrapperApp.xcworkspace" "$app_xcworkspace"
fi

echo "success!"
