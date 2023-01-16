#!/bin/bash
#
# Generate 'macOS' project for %SharedlibName%.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_path="${HOME}/Qt/5.15.2/clang_64"
qt_osx_dir="$bash_dir/build/mac_x64"
cmake -S "$bash_dir" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_PREFIX_PATH=${qt_osx_path}

app_xcodeproj_name="sharedlib_name.xcodeproj"
app_xcodeproj="${bash_dir}/${app_xcodeproj_name}"
if ! [ -h "${app_xcodeproj}" ]; then
  ln -s "${qt_osx_dir}/${app_xcodeproj_name}" "${app_xcodeproj}"
fi

echo "success!"