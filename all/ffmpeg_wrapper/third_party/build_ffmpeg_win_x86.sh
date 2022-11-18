#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# x264
${bash_dir}/x264/win/build_x86.sh
# ffmpeg
${bash_dir}/ffmpeg/win/build_x86.sh

cd "${bash_dir}"