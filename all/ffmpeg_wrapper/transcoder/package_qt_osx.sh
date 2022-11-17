#!/bin/bash
#
# Generate 'macOS' project for Transcoder.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_path="/Users/xiufeng.liu/Qt/5.15.2/clang_64"
qt_osx_package_dir="${bash_dir}/package"
qt_osx_dir="${qt_osx_package_dir}/mac_x64"

# generate
cmake -S "$bash_dir" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_PREFIX_PATH=${qt_osx_path} -DQT_DEP_DIR=${qt_osx_path}
# build
cmake --build "$qt_osx_dir" -v --config Release
# codesign
codesign --deep --force --sign - "${qt_osx_dir}/bin/Release/transcoder_app.app"
# package
# cpack -G DragNDrop -C Release -B "${qt_osx_package_dir}" --config "$qt_osx_dir/CPackConfig.cmake" --verbose
cpack -G DragNDrop -C Release -B "${qt_osx_package_dir}" --config "$qt_osx_dir/CPackConfig.cmake"

echo "success!"
