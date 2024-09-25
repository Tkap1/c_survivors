@echo off
cls
setlocal

for %%a in (%*) do set "%%a=1"

if not "%release%"=="1" if not "%optimized_debug%"=="1" set debug=1

set cl_common=cl -nologo ..\src\main.c -W4 -I..\src\include
set link_common=-link ..\SDL3.lib -INCREMENTAL:NO
set cl_debug=-Od -Zi -Dm_debug=1 -MTd
set cl_optimized_debug=-O2 -Zi -Dm_debug=1 -MTd
set cl_release=-O2 -MT

if "%debug%"=="1" echo [Building debug]
if "%optimized_debug%"=="1" echo [Building optimized debug]
if "%release%"=="1" echo [Building release]

pushd build
	if "%debug%"=="1" %cl_common% %cl_debug% %link_common% || exit /b 1
	if "%optimized_debug%"=="1" %cl_common% %cl_optimized_debug% %link_common% || exit /b 1
	if "%release%"=="1" %cl_common% %cl_release% %link_common% || exit /b 1
popd

copy build\main.exe main.exe > NUL