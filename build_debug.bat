@echo off

cls

REM Configure a debug build
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug

REM Actually build the binaries
cmake --build build/

pause