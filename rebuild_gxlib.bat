@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: rebuild_gxlib.bat
::
:: Double-click to do a clean rebuild of GXLib.
::
:: DELETES:
::   %~dp0build\   (CMake build output of GXLib itself)
:: PRESERVES (never touched):
::   All your custom project folders (MyGame\, TEST_1111\, etc.)
::   Their individual build\ subfolders
::   Any files you edited: main.cpp, Assets\, etc.
::   git history, the GXLib source code
::
:: Use this when: git pull, switching Debug/Release, or build cache issues.
:: For just opening the existing solution, use open_examples.bat.
:: ============================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
cd /d "%ROOT%"

echo.
echo =============================================
echo   GXLib - Clean Rebuild
echo =============================================
echo.
echo This will DELETE:
echo   %ROOT%\build\
echo.
echo This will PRESERVE:
echo   Your project folders (MyGame\, etc.)
echo   Your source files (main.cpp, Assets\, etc.)
echo   Git history and all GXLib source code.
echo.

set /p "CONFIRM=Proceed with clean rebuild? (y/N): "
if /i not "%CONFIRM%"=="y" (
    echo Cancelled. No changes were made.
    pause
    exit /b 0
)

:: ---------------------------------------------------------------------------
:: [1/4] CMake check
:: ---------------------------------------------------------------------------
echo.
echo [1/4] Checking CMake...
where cmake >nul 2>&1
if errorlevel 1 (
    echo   [ERROR] CMake not found.
    echo   Install: https://cmake.org/download/  or  winget install Kitware.CMake
    pause
    exit /b 1
)
echo   CMake ... OK

:: ---------------------------------------------------------------------------
:: [2/4] Delete build\  (flat logic to avoid nested-if pitfalls)
:: ---------------------------------------------------------------------------
echo.
echo [2/4] Deleting build folder...

if not exist "%ROOT%\build" goto :skip_delete

rmdir /s /q "%ROOT%\build" 2>nul
if exist "%ROOT%\build" (
    echo   [ERROR] Failed to delete build\ folder.
    echo   It may be locked by Visual Studio. Close VS and try again.
    pause
    exit /b 1
)
echo   build\ deleted ... OK
goto :after_delete

:skip_delete
echo   build\ did not exist (nothing to delete).

:after_delete

:: ---------------------------------------------------------------------------
:: [3/4] Re-run CMake configure
:: ---------------------------------------------------------------------------
echo.
echo [3/4] Running CMake configure...
echo   (Re-fetching dependencies, so give it a moment)
echo.

cmake -B build -S . -DGX_BUILD_EXAMPLES=ON
if errorlevel 1 (
    echo.
    echo   [ERROR] CMake configure failed.
    echo   See the output above for details.
    pause
    exit /b 1
)

echo.
echo   Configure ... OK

:: ---------------------------------------------------------------------------
:: [4/4] Launch Visual Studio
:: ---------------------------------------------------------------------------
echo.
echo [4/4] Launching Visual Studio...
echo.

if not exist "build\GXLib.sln" (
    echo   [ERROR] build\GXLib.sln was not created.
    pause
    exit /b 1
)

start "" "build\GXLib.sln"

echo Done!
echo.
echo In Visual Studio:
echo   - Build ^> Build Solution  (Ctrl+Shift+B) to compile everything
echo   - Or right-click a single example and "Build"
echo.
echo Your personal project folders were NOT touched.
echo.
pause
endlocal
exit /b 0
