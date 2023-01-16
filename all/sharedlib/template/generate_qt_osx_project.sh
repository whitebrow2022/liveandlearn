#!/bin/bash
#
# Generate 'macOS' project for %SharedlibName%.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_path="${HOME}/Qt/5.15.2/clang_64"
qt_osx_dir="$bash_dir/build/macos_qt/apps/sharedlib_name_app"
cmake -S "$bash_dir/apps/sharedlib_name_app/qt/SharedlibNameApp" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DMACOS:BOOL=ON -DCMAKE_PREFIX_PATH=${qt_osx_path}

app_xcodeproj="$bash_dir/QtApp.xcodeproj"
if ! [ -h "$app_xcodeproj" ]; then
  ln -s "$qt_osx_dir/sharedlib_name_app.xcodeproj" "$app_xcodeproj"
fi

echo "success!"