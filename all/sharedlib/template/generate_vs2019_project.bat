@cls
@title Generate 'Visual Studio 16 2019' solution for %SharedlibName%
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

echo Generate sharedlib_name_example
set vs_example_dir=%bat_dir%\build\windows\vs2019\examples\sharedlib_name_example
cmake -S "%bat_dir%\examples\sharedlib_name_example" -B "%vs_example_dir%" -G "Visual Studio 16 2019" -A Win32 -T v142 -DWINDOWS:BOOL=ON
set cmake_generate_err=%ERRORLEVEL%
if %cmake_generate_err% NEQ 0 (
  exit /b %cmake_generate_err%
)
set vs_example_sln=%bat_dir%\vs2019_example.sln
if exist %vs_example_sln% (
  echo '%vs_example_sln%' exist
) else (
  mklink "%vs_example_sln%" "%vs_example_dir%\sharedlib_name_example.sln"
)

echo "..."
