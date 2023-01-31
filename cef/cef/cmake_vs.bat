@cls
@title Generate 'Visual Studio 17 2022' solution
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

@set PATH=%PATH%;C:\Program Files\CMake\bin

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

echo Generate 
set qt_msvc_path="C:\Qt\Qt5.15.2\5.15.2\msvc2019"
set vs_qt_build_dir=%bat_dir%\build\win_x86
set src_code_dir=%bat_dir%
cmake -S %src_code_dir% -B "%vs_qt_build_dir%" -G "Visual Studio 17 2022" -A WIN32 -T v143 -DCMAKE_PREFIX_PATH=%qt_msvc_path% -DQT_MSVC_DIR="%qt_msvc_path%"
set cmake_generate_err=%ERRORLEVEL%
if %cmake_generate_err% NEQ 0 (
  exit /b %cmake_generate_err%
)

echo "..."
