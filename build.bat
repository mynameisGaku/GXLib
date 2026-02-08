@echo off
cd /d "%~dp0"

if not exist build (
    echo [CMake Configure]
    cmake -S . -B build
    if errorlevel 1 (
        echo Configure failed.
        pause
        exit /b 1
    )
)

echo [Build Debug]
cmake --build build --config Debug
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)

echo Build succeeded.
pause
