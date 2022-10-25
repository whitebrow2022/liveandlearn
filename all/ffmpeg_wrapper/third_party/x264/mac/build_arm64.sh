#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

x264_repo=${bash_dir}/../../externals/x264
out_dir=${bash_dir}/../out

# cd to x264 source code dir
cd ${x264_repo}

# build and install x264
# x264: m1 build m1
./configure --disable-asm --enable-static --prefix=${out_dir}/mac_arm64
make install

# cd to curr bash dir
cd ${bash_dir}
