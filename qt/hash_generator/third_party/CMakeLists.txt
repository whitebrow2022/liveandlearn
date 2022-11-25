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
set(boringssl_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/externals/boringssl")
set(boringssl_REPO "https://boringssl.googlesource.com/boringssl")
set(boringssl_BRANCH_NAME "master")
git_clone_and_update(${boringssl_REPO} ${boringssl_LOCATION} ${boringssl_BRANCH_NAME})
add_subdirectory(externals/boringssl)
