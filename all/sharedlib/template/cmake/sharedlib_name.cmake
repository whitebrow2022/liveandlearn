
#
option(USE_SHAREDLIB_NAME_SHARED_LIBS "${project_name} use sharedlib_name by shared library." OFF)
option(USE_SHAREDLIB_NAME_STATIC_LIBS "${project_name} use sharedlib_name by static library." OFF)
option(USE_SHAREDLIB_NAME_OBJECT_LIBS "${project_name} use sharedlib_name by object library." ON)

if (USE_SHAREDLIB_NAME_SHARED_LIBS)
  message(STATUS "======>> ${project_name} use sharedlib_name by shared library.")
  set(common_name sharedlib_name_lib)
  if(IOS OR MACOS OR ANDROID)
    set(common_name sharedlib_name_common_lib)  
  endif()
elseif(USE_SHAREDLIB_NAME_STATIC_LIBS)
  message(STATUS "======>> ${project_name} use sharedlib_name by static library.")
  set(common_name sharedlib_name_static)
  if(IOS OR MACOS OR ANDROID)
    set(common_name sharedlib_name_common_static)  
  endif()
elseif(USE_SHAREDLIB_NAME_OBJECT_LIBS)
  message(STATUS "======>> ${project_name} use sharedlib_name by object library.")
  set(common_name sharedlib_name_object)
  if(IOS OR MACOS OR ANDROID)
    set(common_name sharedlib_name_common_object)  
  endif()
endif()
