cmake_minimum_required(VERSION 3.23)

set(project_name local_fortune_server)

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

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network LinguistTools REQUIRED)

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
)

set_target_properties(${project_name} PROPERTIES 
  AUTOUIC ON
  AUTOMOC ON
  AUTORCC ON
)

target_link_libraries(${project_name} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

set_target_properties(${project_name} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER whitebrow.qdbus_client.com
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
endif()
