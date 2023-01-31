#!/bin/bash
#
# Generate 'macOS' project.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

echo Generate 
qt_osx_path="${QT_DIR}"
xcode_qt_build_dir="${bash_dir}/build/mac_x64"
src_code_dir="${bash_dir}"
cmake -S "${src_code_dir}" -B "${xcode_qt_build_dir}" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64
app_xcodeproj="$bash_dir/fun_cef.xcodeproj"
if ! [ -h "$app_xcodeproj" ]; then
  ln -s "$xcode_qt_build_dir/fun_cef.xcodeproj" "$app_xcodeproj"
fi

echo "success!"
