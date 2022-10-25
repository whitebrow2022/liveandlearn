#!/bin/bash
#
# Copy frameworks for FfmpegWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "copy frameworks ..."

function copy_frameworks() {
  # echo "======: $1; ------: $2"
  if [ -d "$1" ]; then 
    cp -rf "$1" "$2"
  fi
}

dest_dir="${TARGET_BUILD_DIR}/${FRAMEWORKS_FOLDER_PATH}"
if [ -d ${dest_dir} ]; then 
  rm -rf ${dest_dir}
fi
if [ ! -d ${dest_dir} ]; then 
  mkdir -p ${dest_dir}
fi
copy_frameworks "${FFMPEG_WRAPPER_LIB_PATH}/ffmpeg_wrapper_common.framework" "${dest_dir}"
copy_frameworks "${TARGET_BUILD_DIR}/FfmpegWrapper.framework" "${dest_dir}"
