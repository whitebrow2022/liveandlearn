#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

bash ${bash_dir}/download_ffmpeg.sh

# x264
git -C ${bash_dir}/externals/x264 clean -xdf
# ffmpeg
git -C ${bash_dir}/externals/ffmpeg clean -xdf

cd "${bash_dir}"
