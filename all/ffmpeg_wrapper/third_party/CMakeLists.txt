# third_party

function(git_clone_and_update repo location branch)
  if(EXISTS "${location}/.git")
    execute_process(
      COMMAND git -C ${location} pull
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE result
    )
    if(NOT ("${result}" STREQUAL "0"))
      message(FATAL_ERROR "error: git -C ${location} pull failed")
    endif()
  else()
    execute_process(
      COMMAND git clone ${repo} ${location}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE result
    )
    if(NOT ("${result}" STREQUAL "0"))
      message(FATAL_ERROR "error: git clone ${repo} ${location} failed")
    endif()
  endif()
  # checkout
  execute_process(
    COMMAND git -C ${location} checkout ${branch}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE result
  )
  if(NOT ("${result}" STREQUAL "0"))
    message(FATAL_ERROR "error: git -C ${location} checkout ${branch} failed")
  endif()
  # pull
  execute_process(
    COMMAND git -C ${location} pull
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE result
  )
  if(NOT ("${result}" STREQUAL "0"))
    message(FATAL_ERROR "error: git -C ${location} pull failed")
  endif()
endfunction()

# Git clone or update external
#set(EXTERNAL_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/src/external")
#set(EXTERNAL_REPO "https://github.com/kniefliu/external.git")
#set(EXTERNAL_BRANCH_NAME "master")
#git_clone_and_update(${OPENSSL_REPO} ${OPENSSL_LOCATION} ${OPENSSL_BRANCH_NAME})

set(FFMPEG_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/externals/ffmpeg")
set(FFMPEG_REPO "https://github.com/FFmpeg/FFmpeg.git")
set(FFMPEG_BRANCH_NAME "release/5.1")
if(NOT EXISTS "${FFMPEG_LOCATION}/.git")
  git_clone_and_update(${FFMPEG_REPO} ${FFMPEG_LOCATION} ${FFMPEG_BRANCH_NAME})
endif()

set(X264_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/externals/x264")
set(X264_REPO "https://code.videolan.org/videolan/x264.git")
set(X264_BRANCH_NAME "stable")
if(NOT EXISTS "${X264_LOCATION}/.git")
  git_clone_and_update(${X264_REPO} ${X264_LOCATION} ${X264_BRANCH_NAME})
endif()

if(IOS)
  # fix compile error
  # target_compile_options(external PRIVATE -Wno-shorten-64-to-32)
endif()
