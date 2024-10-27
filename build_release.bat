@echo off

cls

REM Configure a debug build
cmake -S . -B build-release/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-release/ --parallel 8

pause