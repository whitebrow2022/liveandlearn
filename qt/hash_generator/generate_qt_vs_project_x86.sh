#!/bin/bash
#
# Generate 'win_x86' project for hash_generator.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate qt win x86 project ..."

qt_msvc_path="C:/Qt/Qt5.15.2/5.15.2/msvc2019"
qt_vs_package_dir="${bash_dir}/build"
vs_qt_dir="${qt_vs_package_dir}/win_x86"

#
export PATH=${PATH}:"/c/Program Files/Go/bin":"/c/nasm32"

# generate
cmake -S "$bash_dir" -B "$vs_qt_dir" -G "Visual Studio 17 2022" -A WIN32 -T v143 -DCMAKE_PREFIX_PATH=${qt_msvc_path} -DQT_MSVC_DIR=${qt_msvc_path} -DCMAKE_SYSTEM_VERSION=10.0.20348.0


echo "success!"
