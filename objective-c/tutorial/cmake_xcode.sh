#!/bin/bash
#
# Generate 'macOS' project.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt macos project ..."

qt_osx_dir="$bash_dir/build/mac_x64"
cmake -S "$bash_dir" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DMACOS:BOOL=ON

app_xcodeproj="$bash_dir/tutorial.xcodeproj"
if ! [ -h "$app_xcodeproj" ]; then
  ln -s "$qt_osx_dir/tutorial.xcodeproj" "$app_xcodeproj"
fi

echo "success!"