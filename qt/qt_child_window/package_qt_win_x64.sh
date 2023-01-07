#!/bin/bash
#
# package 'win_x64' project for qt_child_window.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "package qt win x64 project ..."

qt_lts_dir=/c/Qt/Qt5.15.2/5.15.2/msvc2019_64
if [ -d ${qt_lts_dir} ] 
then
  export QT_DIR=/c/Qt/Qt5.15.2/5.15.2/msvc2019_64
  export QTDIR=/c/Qt/Qt5.15.2/5.15.2/msvc2019_64
else
  echo "need set QT_DIR env"
  exit 0
fi

qt_msvc_path="${QT_DIR}"
qt_vs_package_dir="${bash_dir}/package"
vs_qt_dir="${qt_vs_package_dir}/win_x64"

# generate
cmake -S "$bash_dir" -B "$vs_qt_dir" -G "Visual Studio 17 2022" -A x64 -T v143 -DCMAKE_PREFIX_PATH=${qt_msvc_path} -DQT_DEP_DIR=${qt_msvc_path}
# build
cmake --build "$vs_qt_dir" -v --config Release
# package
nsis_dir="C:/Program Files (x86)/NSIS"
export PATH=${PATH}:"${nsis_dir}"
#cpack -G NSIS -C Release -B "${qt_vs_package_dir}" --config "$vs_qt_dir/CPackConfig.cmake" --debug --verbose
cpack -G NSIS -C Release -B "${qt_vs_package_dir}" --config "$vs_qt_dir/CPackConfig.cmake"

echo "success!"
