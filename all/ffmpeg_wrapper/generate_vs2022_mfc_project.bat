@cls
@title Generate 'Visual Studio 17 2022' solution for FfmpegWrapper mfc app
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

echo Generate ffmpeg_wrapper_mfc_app
set vs_app_dir=%bat_dir%\build\windows\vs2022\apps\ffmpeg_wrapper_mfc_app
cmake -S "%bat_dir%\apps\ffmpeg_wrapper_app\windows\FfmpegWrapperMfcApp" -B "%vs_app_dir%" -G "Visual Studio 17 2022" -A Win32 -T v143 -DWINDOWS:BOOL=ON
set cmake_generate_err=%ERRORLEVEL%
if %cmake_generate_err% NEQ 0 (
  exit /b %cmake_generate_err%
)
set vs_mfc_app_sln=%bat_dir%\vs2022_mfc_app.sln
if exist %vs_mfc_app_sln% (
  echo '%vs_mfc_app_sln%' exist
) else (
  mklink "%vs_mfc_app_sln%" "%vs_app_dir%\ffmpeg_wrapper_mfc_app.sln"
)

echo "..."
