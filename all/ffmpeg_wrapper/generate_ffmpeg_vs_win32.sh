#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo Generate ffmpeg_wrapper_app
vs_dir=${bash_dir}/build/windows/ffmpeg_x86
cmake -S "${bash_dir}/ffmpeg" -B "${vs_dir}" -G "Visual Studio 17 2022" -A Win32 -T v143 -DWINDOWS:BOOL=ON
