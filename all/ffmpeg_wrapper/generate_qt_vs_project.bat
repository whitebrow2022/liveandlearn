@cls
@title Generate 'Visual Studio 17 2022' solution for Notepad qt app
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

echo Generate ffmpeg_wrapper_app
set qt_msvc_path="C:\Qt\Qt5.15.2\5.15.2\msvc2019_64"
set vs_qt_dir=%bat_dir%\build\windows_qt\apps\ffmpeg_wrapper_app
cmake -S "%bat_dir%\apps\ffmpeg_wrapper_app\qt\FfmpegWrapperApp" -B "%vs_qt_dir%" -G "Visual Studio 17 2022" -A x64 -T v143 -DWINDOWS:BOOL=ON -DCMAKE_PREFIX_PATH=%qt_msvc_path% -DQT_MSVC_DIR="%qt_msvc_path%" -DFFMPEG_WRAPPER_BUILD_SHARED_LIBS=ON -DFFMPEG_WRAPPER_BUILD_STATIC_LIBS=OFF -DFFMPEG_WRAPPER_BUILD_OBJECT_LIBS=OFF -DUSE_FFMPEG_WRAPPER_SHARED_LIBS=ON -DUSE_FFMPEG_WRAPPER_STATIC_LIBS=OFF -DUSE_FFMPEG_WRAPPER_OBJECT_LIBS=OFF
set cmake_generate_err=%ERRORLEVEL%
if %cmake_generate_err% NEQ 0 (
  exit /b %cmake_generate_err%
)
set qt_app=%bat_dir%\qt_app.sln
if exist %qt_app% (
  echo '%qt_app%' exist
) else (
  mklink "%qt_app%" "%vs_qt_dir%\ffmpeg_wrapper_app.sln"
)

echo "..."
