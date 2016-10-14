#!/bin/sh

mkdir build
cd build
CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -DGLSLANG_VALIDATOR=$HOME/bin/khronos-sdk/bin/glslangValidator -DVULKAN_LIBRARY_DIR=$HOME/bin/khronos-sdk/lib -DVULKAN_INCLUDE_DIR=$HOME/bin/khronos-sdk/include ../src
cd ..