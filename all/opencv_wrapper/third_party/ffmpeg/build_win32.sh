#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# 1.安装msys2: https://www.msys2.org/; 安装必须的工具pacman -S make pkgconf diffutils

# 2.msvc环境
# 配置msvc环境
${bash_dir}/make_vs2022_env.bat
# 导入msvc环境
source ${bash_dir}/vs_env.sh
# 测试msvc编译器
#cl.exe /?

# 3.安装工具链: pacman -S mingw-w64-x86_64-toolchain
export PATH=$PATH:/c/msys64/mingw64/bin

# 4.构建
cd ${bash_dir}/../externals/ffmpeg

./configure --toolchain=msvc --arch=i686 --disable-x86asm --enable-static --disable-shared --prefix=${bash_dir}/out/win --disable-programs
make
make install

cd "${bash_dir}"