@echo off
cd build
cmake ../ -DCMAKE_BUILD_TYPE="Debug" -G"MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND"
