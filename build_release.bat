@echo off

cls

REM Configure a release build
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Release

REM Actually build the binaries
cmake --build build/

pause