@cls
@title Setup msvc compile env
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

set vcvarsall_path="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %vcvarsall_path% (
  set vcvarsall_path="C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %vcvarsall_path% (
  echo "vcvarsall.bat path not find!"
  pause
  exit 1
)

set old_path=%PATH%

if "%~1"=="amd64" (
  call %vcvarsall_path% amd64 > NUL:
) else (
  call %vcvarsall_path% x86 > NUL:
)

echo #!/bin/bash > %bat_dir%\vs_env.sh
echo export INCLUDE='%INCLUDE%' >> %bat_dir%\vs_env.sh
echo export LIB='%LIB%' >> %bat_dir%\vs_env.sh
echo export LIBPATH='%LIBPATH%'  >> %bat_dir%\vs_env.sh

call set new_path=%%PATH:%old_path%=%%
set new_path=%new_path:C:=/c%
set new_path=%new_path:\=/%
set new_path=%new_path:;=:%
echo export PATH="%new_path%:$PATH" >> %bat_dir%\vs_env.sh
