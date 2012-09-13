#!/bin/sh
mkdir ../build
cd ../build
cmake .. -G"Xcode" -DASMJIT_BUILD_SAMPLES=1
cd ../scripts
