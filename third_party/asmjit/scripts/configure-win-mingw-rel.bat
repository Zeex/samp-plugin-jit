@echo off

set ASMJIT_SCRIPTS_DIR=%CD%
set ASMJIT_BUILD_DIR="build_mingw_rel"

mkdir ..\%ASMJIT_BUILD_DIR%
cd ..\%ASMJIT_BUILD_DIR%
cmake .. -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DASMJIT_BUILD_SAMPLES=1
cd %ASMJIT_SCRIPTS_DIR%

pause
