#!/bin/sh
mkdir ../build
cd ../build
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DASMJIT_BUILD_SAMPLES=1
cd ../scripts
