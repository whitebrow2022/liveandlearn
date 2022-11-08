#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# 1.安装msys2: https://www.msys2.org/; 安装必须的工具pacman -S make pkgconf diffutils

# 2.msvc环境
# 配置msvc环境
${bash_dir}/make_vs2022_env.bat
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

# 3.安装工具链: nasm make pkg-config difftools
nasm_path=${bash_dir}/../../../../../tools/nasm/win32
deps_path=${bash_dir}/../../../../../tools/bash_deps/win
export PATH=${deps_path}:${PATH}:${nasm_path}


# 4.构建
cd ${bash_dir}/../../externals/x264


CC=cl ./configure --enable-static --prefix=${bash_dir}/../out/win_x86
make
make install

# 5.重命名静态库后缀名


cd "${bash_dir}"