#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo Generate ffmpeg_wrapper_app
vs_dir=${bash_dir}/build/macos/ffmpeg
cmake -S "${bash_dir}/ffmpeg" -B "${vs_dir}" -G "Xcode" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DMACOS=ON
