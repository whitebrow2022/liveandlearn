@cls
@title Generate 'Visual Studio 17 2022' solution for qt app
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

echo Generate hash_generator

set qt_lts_dir="C:\Qt\Qt5.15.2\5.15.2\msvc2019_64"
if exist %qt_lts_dir%\ (
  set QT_DIR="C:\Qt\Qt5.15.2\5.15.2\msvc2019_64"
  set QTDIR="C:\Qt\Qt5.15.2\5.15.2\msvc2019_64"
) else (
  echo "need set QT_DIR env"
  exit 0
)

set PATH=%PATH%;C:\Program Files\Go\bin;C:\nasm

set qt_msvc_path=%QT_DIR%
set vs_qt_dir=%bat_dir%\build\win_x64
rem fix “stdalign.h”: No such file or directory
rem refer: https://devblogs.microsoft.com/cppblog/c11-and-c17-standard-support-arriving-in-msvc/
cmake -S %bat_dir% -B %vs_qt_dir% -G "Visual Studio 17 2022" -A x64 -T v143 -DCMAKE_PREFIX_PATH=%qt_msvc_path% -DQT_MSVC_DIR="%qt_msvc_path%" -DCMAKE_SYSTEM_VERSION=10.0.20348.0
set cmake_generate_err=%ERRORLEVEL%
if %cmake_generate_err% NEQ 0 (
  exit /b %cmake_generate_err%
)
set qt_app=%bat_dir%hash_generator.sln
if exist %qt_app% (
  echo '%qt_app%' exist
) else (
  mklink "%qt_app%" "%vs_qt_dir%\hash_generator.sln"
)

echo "..."
