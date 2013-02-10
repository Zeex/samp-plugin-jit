mkdir ..\build
cd ..\build
cmake .. -G"Visual Studio 10 Win64" -DASMJIT_BUILD_SAMPLES=1
cd ..\scripts
pause
