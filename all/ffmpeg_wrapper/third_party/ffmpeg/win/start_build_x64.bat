@cls
@title Setup msvc compile env
@echo off

rem The %~dp0 (that is a zero) variable when referenced within a Windows batch file will expand to the drive letter and path of that batch file.
set bat_dir=%~dp0%

rem The variables %0-%9 refer to the command line parameters of the batch file.
rem %1-%9 refer to command line arguments after the batch file name. %0 refers to the batch file itself.

set batch_file=%0

rem method 1: use msys2_shell.cmd
set msys2_path=C:\msys64\msys2_shell.cmd
if exist %msys2_path% (
  rem msys2 exist
) else (
  echo %msys2_path% not exist!
  exit 0
)
rem call msys2 to execute build bash
call %msys2_path% "%bat_dir%build_x64.sh"

rem rem method 2:
rem set msys2_path=C:\msys64\usr\bin\mintty.exe
rem if exist %msys2_path% (
rem   rem msys2 exist
rem ) else (
rem   echo %msys2_path% not exist!
rem   exit 0
rem )
rem 
rem rem call msys2 to execute build bash
rem %msys2_path% "/usr/bin/bash" -l "%bat_dir%build_x64.sh"
