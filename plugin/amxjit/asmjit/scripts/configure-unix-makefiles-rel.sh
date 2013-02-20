#!/bin/sh

ASMJIT_SCRIPTS_DIR=`pwd`
ASMJIT_BUILD_DIR="build_makefiles_rel"

mkdir ../${ASMJIT_BUILD_DIR}
cd ../${ASMJIT_BUILD_DIR}
cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DASMJIT_BUILD_SAMPLES=1
cd ${ASMJIT_SCRIPTS_DIR}
