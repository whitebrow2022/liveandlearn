cmake_minimum_required(VERSION 3.23)

set(project_name fun_qt_app)

project(${project_name} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Separate multiple Projects and put them into folders which are on top-level.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# How do I make CMake output into a 'bin' dir?
#   The correct variable to set is CMAKE_RUNTIME_OUTPUT_DIRECTORY.
#   We use the following in our root CMakeLists.txt:
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "CMAKE_PREFIX_PATH is ${CMAKE_PREFIX_PATH}")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

# base
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../fun_qt_base/include)

file(GLOB_RECURSE app_src ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX src FILES ${app_src})
file(GLOB_RECURSE app_ui ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX ui FILES ${app_ui})
file(GLOB_RECURSE app_res ${CMAKE_CURRENT_SOURCE_DIR}/*.ico ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX res FILES ${app_res})

#qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(${project_name}
  ${app_src}
  ${app_ui}
  ${app_res}
  ${qm_files}
  ${project_name}.qrc
)

set_target_properties(${project_name} PROPERTIES 
  AUTOUIC ON
  AUTOMOC ON
  AUTORCC ON
)

target_link_libraries(${project_name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${project_name} PRIVATE fun_qt_base)

set_target_properties(${project_name} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER whitebrow.fun_qt.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
  list(APPEND qt_dll_lists "Qt5Core" "Qt5Gui" "Qt5Widgets" "Qt5WinExtras" 
      "Qt5Concurrent" 
      "Qt5Network" 
      "Qt5WebEngineCore" "Qt5WebEngine" "Qt5WebEngineWidgets" "Qt5Quick" "Qt5QuickWidgets" "Qt5PrintSupport" "Qt5Positioning" "Qt5WebChannel" "Qt5Qml" "Qt5QmlModels"
      )
  
  # Debug configuration, copy qt runtime to bin
  if (PACKAGE)
    foreach(qt_dll_name IN LISTS qt_dll_lists)
      file(COPY ${QT_MSVC_DIR}/bin/${qt_dll_name}d.dll DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug")
    endforeach()
    file(COPY "${QT_MSVC_DIR}/plugins/platforms/qwindowsd.dll" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/plugins/platforms/")
    file(COPY "${QT_MSVC_DIR}/bin/QtWebEngineProcessd.exe" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/")
    file(COPY "${QT_MSVC_DIR}/resources/icudtl.dat" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/resources/")
    file(COPY "${QT_MSVC_DIR}/resources/qtwebengine_resources.pak" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/resources/")
    file(COPY "${QT_MSVC_DIR}/translations/qtwebengine_locales" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/translations/")
  endif()
endif()

if(WIN32)
  set_target_properties(${project_name} PROPERTIES 
    VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${QT_MSVC_DIR}/bin;${QT_MSVC_DIR}/plugins;${QT_MSVC_DIR}/translations;${QT_MSVC_DIR}/resources"
    )

  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${project_name})

  # package
  set(app_bin_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
  if (EXISTS ${QT_DEP_DIR})
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
    set(CPACK_NSIS_DISPLAY_NAME fun_qt)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL TRUE)
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        CreateShortCut \\\"$DESKTOP\\\\${project_name}.lnk\\\" \\\"$INSTDIR\\\\${project_name}.exe\\\"
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        Delete \\\"$DESKTOP\\\\${project_name}.lnk\\\"
    ")
    add_custom_command(TARGET ${project_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/bin/Qt5Core.dll "${app_bin_dir}/Qt5Core.dll"
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/bin/Qt5Widgets.dll "${app_bin_dir}/Qt5Widgets.dll"
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/bin/Qt5Network.dll "${app_bin_dir}/Qt5Network.dll"
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/bin/Qt5Gui.dll "${app_bin_dir}/Qt5Gui.dll"
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/bin/Qt5WinExtras.dll "${app_bin_dir}/Qt5WinExtras.dll"
      COMMAND ${CMAKE_COMMAND} -E copy ${QT_DEP_DIR}/plugins/platforms/qwindows.dll "${app_bin_dir}/plugins/platforms/qwindows.dll"
      )

    ## Destination paths below are relative to ${CMAKE_INSTALL_PREFIX}
    install(TARGETS ${project_name} 
      RUNTIME DESTINATION . 
      )
    install(
      DIRECTORY "${app_bin_dir}/"
      DESTINATION .
      FILES_MATCHING PATTERN "*.dll"
      )
    
    set(CPACK_GENERATOR "NSIS")
    include(CPack)
  endif()
endif()

if(APPLE)
  # package
  set_target_properties(${project_name} PROPERTIES
    INSTALL_RPATH @executable_path/../Frameworks  # Assumes macOS bundle layout
    # CMake 3.20 or later required for the following
  )
  set(app_bundle_contents_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${project_name}.app/Contents")
  set(app_bundle_frameworks_dir "${app_bundle_contents_dir}/Frameworks")
  # copy dynamic loaded libs
  add_custom_command(TARGET ${project_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/libfun_qt_base.dylib ${app_bundle_frameworks_dir}/libfun_qt_base.dylib
    )
  if (EXISTS ${QT_DEP_DIR})
    add_custom_command(TARGET ${project_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtCore.framework "${app_bundle_frameworks_dir}/QtCore.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtWidgets.framework "${app_bundle_frameworks_dir}/QtWidgets.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtNetwork.framework "${app_bundle_frameworks_dir}/QtNetwork.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtGui.framework "${app_bundle_frameworks_dir}/QtGui.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtDBus.framework "${app_bundle_frameworks_dir}/QtDBus.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/lib/QtPrintSupport.framework "${app_bundle_frameworks_dir}/QtPrintSupport.framework"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/plugins/platforms "${app_bundle_contents_dir}/plugins/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_DEP_DIR}/plugins/styles "${app_bundle_contents_dir}/plugins/styles"
      )

    ## Destination paths below are relative to ${CMAKE_INSTALL_PREFIX}
    install(TARGETS ${project_name} 
      BUNDLE DESTINATION . COMPONENT Runtime
      )
    ## Note Mac specific extension .app
    #set(APPS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${project_name}.app")
    #
    ## Directories to look for dependencies
    #set(DIRS ${CMAKE_PREFIX_PATH})
    #install(CODE "include(BundleUtilities) 
    #  fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")
    
    set(CPACK_GENERATOR "DRAGNDROP")
    include(CPack)
  endif()
endif()
