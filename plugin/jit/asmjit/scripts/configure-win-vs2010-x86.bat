@echo off

set ASMJIT_SCRIPTS_DIR=%CD%
set ASMJIT_BUILD_DIR="build_vs2010_x86"

mkdir ..\%ASMJIT_BUILD_DIR%
cd ..\%ASMJIT_BUILD_DIR%
cmake .. -G"Visual Studio 10" -DASMJIT_BUILD_SAMPLES=1
cd %ASMJIT_SCRIPTS_DIR%

pause
