# ffmpeg

## 编译

### msvc编译

1. 安装msys2: 
   - [msys2](https://www.msys2.org)
   - 安装必须的工具 `pacman -S make pkgconf diffutils`
   - or git bash构建
      - 安装make: 参考[make-4.4-without-guile-w32-bin.zip](https://gist.github.com/evanwill/0207876c3243bbb6863e65ec5dc3f058)
         - 解压后，拷贝到`C:\Program Files\Git\mingw64`，注意：不要替换
      - 安装pkgconfig: [](https://stackoverflow.com/questions/1710922/how-to-install-pkg-config-in-windows)
         - 解压后，拷贝[pkg-config-lite-0.28-1_bin-win32.zip]()到安装 
      - 安装diffutils
         - diffutils-2.8.7-1-bin.zip和diffutils-2.8.7-1-dep.zip
2. msvc环境
   - 配置msvc环境
      ```bat
      @cls
      @title Setup msvc compile env
      @echo off
      
      set bat_dir=%~dp0%
      set vcvarsall_path="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
      
      set old_path=%PATH%
      
      call %vcvarsall_path% x86 > NUL:
      
      echo #!/bin/bash > vs_env.sh
      echo export INCLUDE='%INCLUDE%' >> vs_env.sh
      echo export LIB='%LIB%' >> vs_env.sh
      echo export LIBPATH='%LIBPATH%'  >> vs_env.sh
      
      call set new_path=%%PATH:%old_path%=%%
      set new_path=%new_path:C:=/c%
      set new_path=%new_path:\=/%
      set new_path=%new_path:;=:%
      echo export PATH="%new_path%:$PATH" >> vs_env.sh
      ```
   - 导入msvc环境: `source ${bash_dir}/vs_env.sh`
   - 测试msvc编译器: `cl.exe /?`

3. 安装工具链: pacman -S mingw-w64-x86_64-toolchain

4. 构建
   ```bash
   cd ffmpeg
   ./configure --toolchain=msvc --arch=i386 --disable-x86asm --prefix=/c/ffmpeg
   make
   make install
   ```

### mac编译

1. 安装pkg-config: `brew install pkg-config`

### ffmpeg转码流程


#### 参考

1. https://trac.ffmpeg.org/wiki/CompilationGuide
2. https://www.ffmpeg.org/platform.html#Microsoft-Visual-C_002b_002b-or-Intel-C_002b_002b-Compiler-for-Windows
3. https://blog.csdn.net/sinat_31066863/article/details/113135326
4. [8K hevc](https://lf3-cdn-tos.bytegoofy.com/obj/tcs-client/resources/hevc_8k60P_bilibili_1.mp4)
