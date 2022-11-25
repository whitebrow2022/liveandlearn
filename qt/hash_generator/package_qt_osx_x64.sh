#!/bin/bash
#
# package 'macOS' project for hash_generator_app.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "package qt macos project ..."

qt_lts_dir=${QT_DIR}
if [ -d ${qt_lts_dir} ]; then
  echo "qt env is ${qt_lts_dir}"
else
  qt_lts_dir=${QTDIR}
  if [ -d ${qt_lts_dir} ] 
  then
    echo "qt env is ${qt_lts_dir}"
  else
    echo "need set QT_DIR or QTDIR env"
    exit 0
  fi
fi


qt_osx_path="${qt_lts_dir}"
qt_osx_package_dir="${bash_dir}/package"
qt_osx_dir="${qt_osx_package_dir}/mac_x64"

# generate
cmake -S "$bash_dir" -B "$qt_osx_dir" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_PREFIX_PATH=${qt_osx_path} -DQT_DEP_DIR=${qt_osx_path}
# build
cmake --build "$qt_osx_dir" -v --config Release
# codesign
codesign --deep --force --sign - "${qt_osx_dir}/bin/Release/hash_generator_app.app"
# package
# cpack -G DragNDrop -C Release -B "${qt_osx_package_dir}" --config "$qt_osx_dir/CPackConfig.cmake" --verbose
cpack -G DragNDrop -C Release -B "${qt_osx_package_dir}" --config "$qt_osx_dir/CPackConfig.cmake"

echo "success!"
