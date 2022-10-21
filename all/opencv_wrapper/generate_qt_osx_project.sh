#!/bin/bash
#
# Generate 'macOS' project for OpencvWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_path="/Users/xiufeng.liu/Qt/5.15.2/clang_64"
qt_osx_dir="$bash_dir/build/macos_qt/apps/opencv_wrapper_app"
cmake -S "$bash_dir/apps/opencv_wrapper_app/qt/OpencvWrapperApp" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DMACOS:BOOL=ON -DCMAKE_PREFIX_PATH=${qt_osx_path} -DBUILD_opencv_dnn=OFF

app_xcodeproj="$bash_dir/QtApp.xcodeproj"
if ! [ -h "$app_xcodeproj" ]; then
  ln -s "$qt_osx_dir/opencv_wrapper_app.xcodeproj" "$app_xcodeproj"
fi

echo "success!"