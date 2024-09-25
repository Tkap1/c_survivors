@echo off

cls

pushd build
	cl -nologo ..\src\main.c -W4 -Od -Zi -I..\src\include -link ..\SDL3.lib
popd

set error_level=%ERRORLEVEL%
copy build\main.exe main.exe > NUL
@REM if %error_level%==0 (
@REM 	call run.bat
@REM )