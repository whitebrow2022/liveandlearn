#!/bin/bash
#
# download third party librarys

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

#######################################
# 克隆或更新git仓库.
# Arguments:
#   random base
# Returns:
#   random value.
#######################################
function git_clone_and_update {
    if (( $# < 3 )); then
      return 1
    fi
    local repo=$1
    local location=$2 
    local branch=$3
    local status=0
    if [[ -d "${location}/.git" ]]; then
      # update
      git -C ${location} pull
      status=$?
    else
      # clone
      git clone ${repo} ${location}
      status=$?
    fi
    if (( ${status} != 0 )); then
      return ${status}
    fi
    git -C ${location} checkout ${branch}
    status=$?
    if (( ${status} != 0 )); then
      return ${status}
    fi
    # update
    git -C ${location} pull
    status=$?
    if (( ${status} != 0 )); then
      return ${status}
    fi
    return 0
}

ffmpeg_location="${bash_dir}/externals/ffmpeg"
ffmpeg_repo="https://github.com/FFmpeg/FFmpeg.git"
ffmpeg_branch_name="release/5.1"
if [[ ! -d "${ffmpeg_location}/.git" ]]; then
  git_clone_and_update ${ffmpeg_repo} ${ffmpeg_location} ${ffmpeg_branch_name}
  echo status is $?
fi
x264_location="${bash_dir}/externals/x264"
x264_repo="https://code.videolan.org/videolan/x264.git"
x264_branch_name="stable"
if [[ ! -d "${x264_location}/.git" ]]; then
  git_clone_and_update ${x264_repo} ${x264_location} ${x264_branch_name}
  echo status is $?
fi
