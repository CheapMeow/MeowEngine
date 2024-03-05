@echo off

cls

REM Configure a debug build
cmake -S . -B build/

REM Actually build the binaries
cmake --build build/ --config Debug

pause