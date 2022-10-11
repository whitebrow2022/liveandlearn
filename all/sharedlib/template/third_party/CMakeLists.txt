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

if(IOS)
  # fix compile error
  # target_compile_options(external PRIVATE -Wno-shorten-64-to-32)
endif()
