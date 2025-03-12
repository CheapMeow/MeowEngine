@echo off

cls

REM Configure a debug build
cmake -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=EditorDebug
cmake --build build/ --parallel 8 --config EditorDebug

pause