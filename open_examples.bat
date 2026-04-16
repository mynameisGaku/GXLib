@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: open_examples.bat
::
:: Double-click this to:
::   1. (If not yet built) Generate build\GXLib.sln
::   2. Open the solution in Visual Studio
::
:: First run: generates the solution (a few minutes for dependencies).
:: Second run: just opens the existing solution (instant).
::
:: Related:
::   rebuild_gxlib.bat       - clean rebuild when you need to start fresh
::   create_new_project.bat  - create a new game project from the template
:: ============================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
cd /d "%ROOT%"

echo.
echo =============================================
echo   GXLib Examples - Open in Visual Studio
echo =============================================
echo.

:: ---------------------------------------------------------------------------
:: Fast path: solution already exists, just open it
:: ---------------------------------------------------------------------------
if exist "build\GXLib.sln" (
    echo Solution found: build\GXLib.sln
    echo Opening Visual Studio...
    echo.
    start "" "build\GXLib.sln"
    echo Done!
    echo.
    echo If you need to rebuild from scratch, run rebuild_gxlib.bat instead.
    echo.
    pause
    endlocal
    exit /b 0
)

:: ---------------------------------------------------------------------------
:: Slow path: first-time setup
:: ---------------------------------------------------------------------------
echo No build folder found - running first-time setup.
echo.

:: [1/3] CMake check
echo [1/3] Checking CMake...
where cmake >nul 2>&1
if errorlevel 1 (
    echo   [ERROR] CMake not found.
    echo.
    echo   Install CMake from:
    echo     https://cmake.org/download/
    echo.
    echo   Or with winget:
    echo     winget install Kitware.CMake
    echo.
    echo   After install, re-run this batch.
    pause
    exit /b 1
)
echo   CMake ... OK

:: [2/3] Generate solution
echo.
echo [2/3] Generating Visual Studio solution...
echo   (First run takes a few minutes for dependencies)
echo.

cmake -B build -S . -DGX_BUILD_EXAMPLES=ON
if errorlevel 1 (
    echo.
    echo   [ERROR] CMake configure failed.
    echo.
    echo   Possible causes:
    echo     - Visual Studio 2022 not installed
    echo       https://visualstudio.microsoft.com/downloads/
    echo     - C++ workload missing in Visual Studio
    echo       (Visual Studio Installer: add "Desktop development with C++")
    echo     - No internet connection (dependency fetch failed)
    echo.
    pause
    exit /b 1
)

echo.
echo   Solution generated ... OK

:: [3/3] Launch VS
echo.
echo [3/3] Launching Visual Studio...
echo.

if not exist "build\GXLib.sln" (
    echo   [ERROR] build\GXLib.sln not found.
    pause
    exit /b 1
)

start "" "build\GXLib.sln"

echo Done!
echo.
echo Next steps:
echo   1. In Solution Explorer, expand the "Examples" folder
echo   2. Right-click any sample (e.g. gxlib_example_01)
echo      and choose "Set as Startup Project"
echo   3. Press F5 to build and run
echo.
echo Recommended starting point: gxlib_example_01 (hello-sprite)
echo.
pause
endlocal
exit /b 0
