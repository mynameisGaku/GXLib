@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: create_new_project.bat
::
:: Double-click this to:
::   1. Prompt for a project name
::   2. Copy template/ to a new project folder
::   3. Rewrite CMakeLists project name
::   4. Generate solution and open in Visual Studio
::
:: Run open_examples.bat first to build the SDK.
:: ============================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
cd /d "%ROOT%"

echo.
echo =============================================
echo   GXLib - Create New Project
echo =============================================
echo.

:: ---------------------------------------------------------------------------
:: [1/5] Prerequisites
:: ---------------------------------------------------------------------------
echo [1/5] Checking prerequisites...

where cmake >nul 2>&1
if errorlevel 1 (
    echo   [ERROR] CMake not found. Run open_examples.bat first.
    pause
    exit /b 1
)
echo   CMake ... OK

if not exist "template\main.cpp" (
    echo   [ERROR] template\main.cpp not found.
    pause
    exit /b 1
)
echo   Template ... OK

if not exist "build\GXLib.sln" (
    echo   [WARN] build\GXLib.sln not found.
    echo   Recommended: run open_examples.bat first to build GXLib.
    echo   You can continue, but GXLib will also be built by this script.
    echo.
)

:: ---------------------------------------------------------------------------
:: [2/5] Project name
:: ---------------------------------------------------------------------------
echo.
echo [2/5] Enter your project name
echo   (ASCII letters, digits, and underscore only. Example: MyGame)
echo.
set /p "PROJ_NAME=  Project name: "

if "%PROJ_NAME%"=="" (
    echo   [ERROR] Empty name.
    pause
    exit /b 1
)

echo %PROJ_NAME%| findstr /r "[^a-zA-Z0-9_]" >nul
if not errorlevel 1 (
    echo   [ERROR] Only ASCII letters, digits, underscore allowed.
    echo   You entered: %PROJ_NAME%
    pause
    exit /b 1
)

if exist "%PROJ_NAME%" (
    echo   [WARN] Folder '%PROJ_NAME%' already exists.
    set /p "OVERWRITE=  Overwrite? (y/N): "
    if /i not "!OVERWRITE!"=="y" (
        echo   Cancelled.
        pause
        exit /b 0
    )
    echo   Removing existing folder...
    rmdir /s /q "%PROJ_NAME%"
)

:: ---------------------------------------------------------------------------
:: [3/5] Copy template
:: ---------------------------------------------------------------------------
echo.
echo [3/5] Copying template...

xcopy /E /I /Q /Y "template" "%PROJ_NAME%" >nul
if errorlevel 1 (
    echo   [ERROR] Copy failed.
    pause
    exit /b 1
)

:: Rename project in CMakeLists.txt via PowerShell
:: Template uses project(MyGame) and ${PROJECT_NAME} everywhere else,
:: so only project(MyGame) needs to be replaced.
set "CMAKE_FILE=%PROJ_NAME%\CMakeLists.txt"
if exist "%CMAKE_FILE%" (
    powershell -NoProfile -Command "$content = Get-Content -Raw '%CMAKE_FILE%'; $content = $content -replace 'project\(MyGame', 'project(%PROJ_NAME%'; Set-Content -Path '%CMAKE_FILE%' -Value $content -NoNewline -Encoding UTF8"
)

echo   Created '%PROJ_NAME%\' ... OK

:: ---------------------------------------------------------------------------
:: [4/5] Generate solution
:: ---------------------------------------------------------------------------
echo.
echo [4/5] Generating solution...
echo.

cd "%PROJ_NAME%"
cmake -B build -S . -DGXLib_DIR="%ROOT%\cmake"
set "CMAKE_RESULT=%errorlevel%"
cd "%ROOT%"

if not "%CMAKE_RESULT%"=="0" (
    echo.
    echo   [WARN] Standalone configure failed.
    echo.
    echo   Alternative: open %ROOT%\build\GXLib.sln
    echo   and add %PROJ_NAME% folder as an existing project.
    echo.
    pause
    exit /b 1
)

echo   Solution generated ... OK

:: ---------------------------------------------------------------------------
:: [5/5] Launch Visual Studio
:: ---------------------------------------------------------------------------
echo.
echo [5/5] Launching Visual Studio...
echo.

set "NEW_SLN=%ROOT%\%PROJ_NAME%\build\%PROJ_NAME%.sln"
if not exist "%NEW_SLN%" (
    for %%f in ("%ROOT%\%PROJ_NAME%\build\*.sln") do set "NEW_SLN=%%f"
)

if exist "%NEW_SLN%" (
    start "" "%NEW_SLN%"
    echo Done! Visual Studio is opening.
) else (
    echo   [WARN] .sln not found. Check %PROJ_NAME%\build\
)

echo.
echo Next steps:
echo   1. In Visual Studio, right-click the %PROJ_NAME% project
echo   2. Choose "Set as Startup Project"
echo   3. Press F5 to build and run
echo.
echo Edit %PROJ_NAME%\main.cpp to write your game.
echo See examples/ for reference.
echo.
pause
endlocal
exit /b 0
