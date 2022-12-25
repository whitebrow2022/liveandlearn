#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

bash ${bash_dir}/download_ffmpeg.sh
bash ${bash_dir}/clean_ffmpeg.sh

# x264
${bash_dir}/x264/mac/build_x64.sh
# ffmpeg
${bash_dir}/ffmpeg/mac/build_x64.sh

cd "${bash_dir}"