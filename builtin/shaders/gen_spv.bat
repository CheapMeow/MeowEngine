@echo off
REM Loop through all .vert and .frag files in the current directory
for %%f in (*.vert *.frag) do (
    glslangValidator -V %%f -o %%f.spv
    if errorlevel 1 (
        echo error
    ) else (
        echo success
    )
)
pause
