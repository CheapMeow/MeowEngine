@echo off

cls
cmake -S . -B build
cmake --build build

pause