@echo off

cls

REM Configure a release build
cmake -S . -B build/

REM Actually build the binaries
cmake --build build/ --config Release --parallel 8

pause