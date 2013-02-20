@echo off

set ASMJIT_SCRIPTS_DIR=%CD%
set ASMJIT_BUILD_DIR="build_borland_dbg"

mkdir ..\%ASMJIT_BUILD_DIR%
cd ..\%ASMJIT_BUILD_DIR%
cmake .. -G"Borland Makefiles" -DCMAKE_BUILD_TYPE=Debug -DASMJIT_BUILD_SAMPLES=1
cd %ASMJIT_SCRIPTS_DIR%

pause
