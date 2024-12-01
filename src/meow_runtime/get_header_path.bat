@echo off
setlocal enabledelayedexpansion

rem Get the current directory
set "current_dir=%CD%"

rem Recursively find .h and .hpp files and remove the prefix of the current directory,
rem replacing backslashes with forward slashes for directory separators
for /r "%current_dir%" %%f in (*.h *.hpp) do (
    rem Remove the current directory prefix to get the relative path
    set "relative_path=%%f"
    set "relative_path=!relative_path:%current_dir%\=!"

    rem Replace backslashes '\' with forward slashes '/'
    set "relative_path=!relative_path:\=/!"

    rem Output the relative path
    echo !relative_path!
)

endlocal
