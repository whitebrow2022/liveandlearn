#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# 1.安装msys2: https://www.msys2.org/; 安装必须的工具pacman -S make pkgconf diffutils

# 2.msvc环境
# 配置msvc环境
${bash_dir}/make_vs2022_env.bat amd64
ret_val=$?
if [ $ret_val -ne 0 ]; then
  # 配置msvc失败
  echo "setup vs2022 env failed!"
  read -p "Press any key to resume ..."
  exit 0
fi

# 导入msvc环境
source ${bash_dir}/vs_env.sh
# 测试msvc编译器
#cl.exe /?

# 3.安装工具链: nasm
nasm_path=${bash_dir}/../../../../../tools/nasm/win64
deps_path=${bash_dir}/../../../../../tools/bash_deps/win
export PATH=${deps_path}:${PATH}:${nasm_path}

# 4.构建
cd ${bash_dir}/../../externals/ffmpeg

x264_install_dir=${bash_dir}/../../x264/out/win_x64
# fix: x264 not found using pkg-config
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${x264_install_dir}/lib/pkgconfig

# default
# ./configure --toolchain=msvc --target-os=win64 --arch=x86_64 --enable-static --disable-shared --prefix=${bash_dir}/../out/win_x64 --disable-programs --enable-libx264 --enable-gpl
# debug, --disable-asm to fix 'ff_fdct_sse2' unreference
./configure --toolchain=msvc --target-os=win64 --arch=x86_64 --enable-debug --disable-asm --disable-protocol=rtmp --disable-protocol=rtmpe --disable-protocol=rtmps --disable-protocol=rtmpt --disable-protocol=rtmpte --disable-protocol=rtmpts  --disable-optimizations --optflags="-Od" --extra-cflags="-Od" --extra-cxxflags="-Od" --extra-ldflags="//DEBUG" --enable-static --disable-shared --prefix=${bash_dir}/../out/win_x64 --disable-programs --enable-libx264 --enable-gpl
make
make install

# 5.重命名静态库后缀名
cd ${bash_dir}/../out/win_x64/lib
cp -p ./libavcodec.a ./libavcodec.lib
cp -p ./libavdevice.a ./libavdevice.lib
cp -p ./libavfilter.a ./libavfilter.lib
cp -p ./libavformat.a ./libavformat.lib
cp -p ./libavutil.a ./libavutil.lib
cp -p ./libswresample.a ./libswresample.lib
cp -p ./libswscale.a ./libswscale.lib
if [ -f "./libpostproc.a" ]; then
cp -p ./libpostproc.a ./libpostproc.lib
fi

cd "${bash_dir}"