# download cef
#
set(cef_sdk_version 99.2.15+g71e9523+chromium-99.0.4844.84)

if(WIN32)
  set(cef_sdk_platform "windows")
elseif(APPLE)
  set(cef_sdk_platform "macos")
else()
  message(FATAL_ERROR "Unsupported Operating System")
endif()

# set cef sdk package name
set(cef_sdk_workspace       "${CMAKE_CURRENT_SOURCE_DIR}/../dep")

if (APPLE)
  # macosx64
  set(cef_sdk_package_name    "cef_binary_${cef_sdk_version}_${cef_sdk_platform}x64")
else()
  # 
  set(cef_sdk_package_name    "cef_binary_${cef_sdk_version}_${cef_sdk_platform}32")
endif()

# Scan extracted sdk dir
set(cef_sdk_extracted_dir "${cef_sdk_workspace}/${cef_sdk_package_name}")
file(GLOB cef_sdk_dir "${cef_sdk_extracted_dir}")

# Extract CEF binary package
if (NOT EXISTS ${cef_sdk_dir})
  set(cef_local_package_path "${cef_sdk_workspace}/${cef_sdk_package_name}.tar.bz2")

  # if no local cef sdk package file then download it
  if(NOT EXISTS "${cef_local_package_path}")
    set(cef_sdk_download_url "https://cef-builds.spotifycdn.com/${cef_sdk_package_name}.tar.bz2")
    message(STATUS "Downloading CEF binary SDK from ${cef_sdk_download_url}")
    file(DOWNLOAD 
      "${cef_sdk_download_url}"   # url
      "${cef_local_package_path}"  # local path
      SHOW_PROGRESS 
      TLS_VERIFY ON
      STATUS download_result
    )
    list(GET download_result 0 download_result_code)
    list(GET download_result 1 download_result_message)
    if (NOT download_result_code EQUAL 0)
      file(REMOVE "${cef_local_package_path}")
      message(FATAL_ERROR "Failed to download cef binary sdk, error:[${download_result_code}]${download_result_message}")
    endif()
  endif()

  message(STATUS "Extracting cef binary sdk from ${cef_local_package_path}")

  # Extract
  file(ARCHIVE_EXTRACT
    INPUT "${cef_local_package_path}"
    DESTINATION "${cef_sdk_workspace}"
  )

  # Capture the dir name
  file(GLOB cef_sdk_dir "${cef_sdk_extracted_dir}")
endif()

# output
message(STATUS "cef sdk dir: ${cef_sdk_dir}")
