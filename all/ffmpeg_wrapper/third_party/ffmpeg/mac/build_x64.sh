#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

ffmpeg_repo=${bash_dir}/../../externals/ffmpeg
out_dir=${bash_dir}/../out

# cd to ffmpeg source code dir
cd ${ffmpeg_repo}

# x264 dir
x264_install_dir=${bash_dir}/../../x264/out/mac_x64
export PKG_CONFIG_PATH=${x264_install_dir}/lib/pkgconfig:${PKG_CONFIG_PATH}
echo ${PKG_CONFIG_PATH}
# x264 need gpl
# build and install ffmpeg
# m1 build x64
./configure --enable-cross-compile --arch=x86_64 --cc='clang -arch x86_64' --enable-static --disable-shared --disable-debug --disable-doc --disable-x86asm --enable-gpl  --enable-libx264 --disable-programs --prefix=${out_dir}/mac_x64 --disable-zlib --disable-iconv --disable-bzlib
make install

# cd to curr bash dir
cd ${bash_dir}
