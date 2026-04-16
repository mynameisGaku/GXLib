@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

:: ---------------------------------------------------------------------------
:: GXLib SDK セットアップウィザード (スタンドアロン対応)
::
:: このファイル 1 つで:
::   1. 必要なツールを確認・インストール (CMake, VS, Git)
::   2. GXLib ソースを GitHub からクローン (リポジトリ外から実行時)
::   3. SDK をビルド & インストール
::   4. ゲームプロジェクトを作成 (.sln 生成)
::   5. Visual Studio で開く
:: ---------------------------------------------------------------------------

:: ★ リポジトリ URL — フォーク時はここを変更
set "GXLIB_REPO=https://github.com/mynameisGaku/GXLib.git"

:: ===================== 内部変数 (編集不要) =====================
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"
set "CYAN=%ESC%[96m"
set "BOLD=%ESC%[1m"
set "DIM=%ESC%[2m"
set "RESET=%ESC%[0m"

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

:: --- リポジトリ内で実行されているか判定 ---
set "GXLIB_SRC="
set "NEED_GIT=1"
if not exist "%ROOT%\CMakeLists.txt" goto :not_in_repo
if not exist "%ROOT%\GXLib\CMakeLists.txt" goto :not_in_repo
set "GXLIB_SRC=%ROOT%"
set "NEED_GIT=0"
:not_in_repo

:: ===================== バナー =====================
echo.
echo %BOLD%%CYAN%=========================================%RESET%
echo %BOLD%%CYAN%  GXLib SDK セットアップウィザード%RESET%
echo %BOLD%%CYAN%=========================================%RESET%
echo.
if defined GXLIB_SRC (
    echo   %DIM%モード: リポジトリ内実行%RESET%
) else (
    echo   %DIM%モード: スタンドアロン ^(自動クローン^)%RESET%
)
echo.

:: ===================================================================
:: [1/5] 環境チェック
:: ===================================================================
echo %BOLD%[1/5] 環境チェック%RESET%
echo.

:: ----- CMake -----
where cmake >nul 2>&1
if errorlevel 1 goto :cmake_missing
echo   CMake ... %GREEN%OK%RESET%
goto :cmake_ok

:cmake_missing
echo   CMake ... %YELLOW%未検出%RESET%
echo.
echo   winget でインストールします...
echo.
winget install Kitware.CMake --accept-source-agreements --accept-package-agreements
if errorlevel 1 (
    echo.
    echo   %RED%インストール失敗。手動でインストールしてください:%RESET%
    echo   https://cmake.org/download/
    pause
    exit /b 1
)
echo.
echo   PATH を再読み込みしています...
for /f "delims=" %%p in ('cmd /c "where cmake 2>nul"') do set "CMAKE_PATH=%%p"
if defined CMAKE_PATH goto :cmake_refreshed
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "SYS_PATH=%%b"
for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USR_PATH=%%b"
set "PATH=!USR_PATH!;!SYS_PATH!"
where cmake >nul 2>&1
if errorlevel 1 (
    echo   %YELLOW%PATH が反映されていません。PC を再起動してから再実行してください。%RESET%
    pause
    exit /b 1
)
:cmake_refreshed
echo   CMake ... %GREEN%OK%RESET% ^(新規インストール^)

:cmake_ok
for /f "tokens=3" %%v in ('cmake --version 2^>nul ^| findstr /i /c:"cmake version"') do echo   バージョン: %%v

:: ----- Visual Studio -----
echo.
echo   MSBuild ^(Visual Studio^) ...
set "VSWHERE=!ProgramFiles(x86)!\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" set "VSWHERE=!ProgramFiles!\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" goto :vs_missing

"!VSWHERE!" -latest -requires Microsoft.Component.MSBuild -property installationPath > "%TEMP%\gxlib_vs.tmp" 2>nul
set "VS_PATH="
set /p VS_PATH=<"%TEMP%\gxlib_vs.tmp"
del "%TEMP%\gxlib_vs.tmp" >nul 2>&1
if not defined VS_PATH goto :vs_missing

echo   %GREEN%OK%RESET%
echo   パス: !VS_PATH!
goto :vs_ok

:vs_missing
echo   %RED%未検出%RESET%
echo.
echo   %RED%Visual Studio または Build Tools が見つかりません。%RESET%
echo   以下のいずれかをインストールしてください:
echo     - Visual Studio 2022 ^(C++ ワークロード付き^)
echo     - Build Tools for Visual Studio 2022
echo   https://visualstudio.microsoft.com/ja/downloads/
echo.
pause
exit /b 1

:vs_ok

:: ----- Git (スタンドアロン時のみ必須) -----
if "!NEED_GIT!"=="0" goto :git_ok

echo.
echo   Git ...
where git >nul 2>&1
if errorlevel 1 goto :git_missing
echo   %GREEN%OK%RESET%
goto :git_ok

:git_missing
echo   %YELLOW%未検出%RESET%
echo.
echo   winget でインストールします...
echo.
winget install Git.Git --accept-source-agreements --accept-package-agreements
if errorlevel 1 (
    echo.
    echo   %RED%インストール失敗。手動でインストールしてください:%RESET%
    echo   https://git-scm.com/
    pause
    exit /b 1
)
echo.
echo   PATH を再読み込みしています...
for /f "delims=" %%p in ('cmd /c "where git 2>nul"') do set "GIT_PATH=%%p"
if defined GIT_PATH goto :git_refreshed
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "SYS_PATH=%%b"
for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USR_PATH=%%b"
set "PATH=!USR_PATH!;!SYS_PATH!"
where git >nul 2>&1
if errorlevel 1 (
    echo   %YELLOW%PATH が反映されていません。PC を再起動してから再実行してください。%RESET%
    pause
    exit /b 1
)
:git_refreshed
echo   Git ... %GREEN%OK%RESET% ^(新規インストール^)

:git_ok
echo.
echo   %GREEN%環境チェック完了%RESET%
echo.

:: ===================================================================
:: [2/5] GXLib ソース取得
:: ===================================================================
echo %BOLD%[2/5] GXLib ソース取得%RESET%
echo.

if defined GXLIB_SRC (
    echo   %GREEN%リポジトリ検出済み%RESET%: !GXLIB_SRC!
    echo.
    goto :source_ok
)

:: プレースホルダーチェック
echo !GXLIB_REPO! | findstr /i "YOUR_ACCOUNT YOUR_NAME CHANGEME" >nul 2>&1
if not errorlevel 1 (
    echo   %RED%GXLIB_REPO が未設定です。%RESET%
    echo   gxlib_setup.bat 冒頭の GXLIB_REPO を正しい URL に変更してください。
    echo   現在の値: !GXLIB_REPO!
    pause
    exit /b 1
)

set "GXLIB_SRC=%ROOT%\GXLib"
echo   クローン先: !GXLIB_SRC!
echo   リポジトリ: !GXLIB_REPO!
echo.

if exist "!GXLIB_SRC!\CMakeLists.txt" (
    echo   %GREEN%既にクローン済み — スキップ%RESET%
    echo.
    goto :source_ok
)

:: sparse checkout — SDK ビルドに必要なファイルだけ取得
echo   %DIM%必要なファイルだけを取得します (sparse checkout)%RESET%
echo.
git clone --filter=blob:none --no-checkout --depth 1 "!GXLIB_REPO!" "!GXLIB_SRC!"
if errorlevel 1 (
    echo.
    echo   %RED%クローンに失敗しました。URL を確認してください:%RESET%
    echo   !GXLIB_REPO!
    pause
    exit /b 1
)
pushd "!GXLIB_SRC!"
git sparse-checkout init --cone
git sparse-checkout set CMakeLists.txt GXLib extern Shaders cmake template
git checkout
popd
echo.
echo   %GREEN%クローン完了%RESET%
echo.

:source_ok

:: ===================================================================
:: [3/5] SDK ビルド & インストール
:: ===================================================================
echo %BOLD%[3/5] SDK ビルド ^& インストール%RESET%
echo.

:: --- ビルド構成 ---
echo   ビルド構成を選択してください:
echo     1) Debug
echo     2) Release
echo     3) 両方 ^(Debug + Release^)
echo.
set "BUILD_CONFIG=3"
set /p "BUILD_CONFIG=  選択 [1/2/3] (デフォルト: 3): "

if "!BUILD_CONFIG!"=="1" (
    set "CONFIGS=Debug"
    echo   → Debug のみ
) else if "!BUILD_CONFIG!"=="2" (
    set "CONFIGS=Release"
    echo   → Release のみ
) else (
    set "CONFIGS=Debug Release"
    echo   → Debug + Release
)
echo.

:: --- SDK 出力先 ---
set "SDK_DIR=%ROOT%\GXLib-SDK"
set /p "SDK_DIR=  SDK 出力先 (デフォルト: !SDK_DIR!): "
if "!SDK_DIR!"=="" set "SDK_DIR=%ROOT%\GXLib-SDK"
echo   → !SDK_DIR!
echo.

:: --- Configure (スタンドアロン時は SDK のみビルド) ---
set "SDK_ONLY_FLAG="
if "!NEED_GIT!"=="1" set "SDK_ONLY_FLAG=-DGX_SDK_ONLY=ON"
echo   %CYAN%[Configure]%RESET%
cmake -B "!GXLIB_SRC!\build" -S "!GXLIB_SRC!" !SDK_ONLY_FLAG!
if errorlevel 1 (
    echo   %RED%Configure に失敗しました。%RESET%
    pause
    exit /b 1
)
echo   %GREEN%Configure 完了%RESET%
echo.

:: --- Build & Install per config ---
for %%c in (!CONFIGS!) do (
    echo   %CYAN%[Build %%c]%RESET%
    cmake --build "!GXLIB_SRC!\build" --config %%c
    if errorlevel 1 (
        echo   %RED%%%c ビルドに失敗しました。%RESET%
        pause
        exit /b 1
    )
    echo   %GREEN%%%c ビルド完了%RESET%
    echo.

    echo   %CYAN%[Install %%c]%RESET%
    cmake --install "!GXLIB_SRC!\build" --config %%c --prefix "!SDK_DIR!"
    if errorlevel 1 (
        echo   %RED%%%c インストールに失敗しました。%RESET%
        pause
        exit /b 1
    )
    echo   %GREEN%%%c インストール完了%RESET%
    echo.
)

echo   %GREEN%SDK ビルド完了！%RESET%
echo.

:: ===================================================================
:: [4/5] プロジェクト作成
:: ===================================================================
echo %BOLD%[4/5] プロジェクト作成%RESET%
echo.

set "CREATE_PROJECT=Y"
set /p "CREATE_PROJECT=  ゲームプロジェクトを作成しますか? [Y/n]: "
if /i "!CREATE_PROJECT!"=="n" goto :skip_project

echo.

:: --- プロジェクト名 ---
set "PROJECT_NAME=MyGame"
set /p "PROJECT_NAME=  プロジェクト名 (デフォルト: MyGame): "
if "!PROJECT_NAME!"=="" set "PROJECT_NAME=MyGame"

:: 英数字 + アンダースコアのみ
echo !PROJECT_NAME!| findstr /r "^[A-Za-z_][A-Za-z0-9_]*$" >nul 2>&1
if errorlevel 1 (
    echo   %RED%プロジェクト名は英数字とアンダースコアのみ使用できます。%RESET%
    pause
    exit /b 1
)
echo   → !PROJECT_NAME!
echo.

:: --- プロジェクト作成先 (GXLib ソースの親ディレクトリ基準) ---
for %%d in ("!GXLIB_SRC!") do set "PROJECT_PARENT=%%~dpd"
if "!PROJECT_PARENT:~-1!"=="\" set "PROJECT_PARENT=!PROJECT_PARENT:~0,-1!"
set "PROJECT_DIR=!PROJECT_PARENT!\!PROJECT_NAME!"
set /p "PROJECT_DIR=  作成先 (デフォルト: !PROJECT_DIR!): "
if "!PROJECT_DIR!"=="" set "PROJECT_DIR=!PROJECT_PARENT!\!PROJECT_NAME!"
echo   → !PROJECT_DIR!
echo.

:: 既存チェック
if exist "!PROJECT_DIR!\CMakeLists.txt" (
    echo   %YELLOW%フォルダが既に存在します: !PROJECT_DIR!%RESET%
    set "OVERWRITE=n"
    set /p "OVERWRITE=  上書きしますか? [y/N]: "
    if /i not "!OVERWRITE!"=="y" (
        echo   スキップしました。
        goto :skip_project
    )
    echo.
)

:: --- テンプレートをコピー ---
echo   テンプレートをコピーしています...
mkdir "!PROJECT_DIR!" >nul 2>&1
copy /y "!GXLIB_SRC!\template\main.cpp" "!PROJECT_DIR!\main.cpp" >nul
if errorlevel 1 (
    echo   %RED%テンプレートのコピーに失敗しました。%RESET%
    pause
    exit /b 1
)

:: --- CMakeLists.txt 生成 (MyGame → プロジェクト名) ---
echo   CMakeLists.txt を生成しています...
powershell -NoProfile -Command ^
    "(Get-Content '!GXLIB_SRC!\template\CMakeLists.txt' -Encoding UTF8) -replace 'MyGame','!PROJECT_NAME!' | Set-Content '!PROJECT_DIR!\CMakeLists.txt' -Encoding UTF8"
if errorlevel 1 (
    echo   %RED%CMakeLists.txt の生成に失敗しました。%RESET%
    pause
    exit /b 1
)

:: --- main.cpp のウィンドウタイトルを置換 ---
powershell -NoProfile -Command ^
    "(Get-Content '!PROJECT_DIR!\main.cpp' -Encoding UTF8) -replace 'GXLib Hello World','!PROJECT_NAME!' | Set-Content '!PROJECT_DIR!\main.cpp' -Encoding UTF8"

:: --- ソリューション生成 ---
echo   ソリューションを生成しています...
echo.
cmake -B "!PROJECT_DIR!\build" -S "!PROJECT_DIR!" -DGXLib_DIR="!SDK_DIR!\cmake"
if errorlevel 1 (
    echo.
    echo   %RED%ソリューション生成に失敗しました。%RESET%
    pause
    exit /b 1
)

:: --- .sln をプロジェクトルートに配置 (vcxproj パスに build/ を付与) ---
set "SLN_BUILD=!PROJECT_DIR!\build\!PROJECT_NAME!.sln"
set "SLN_ROOT=!PROJECT_DIR!\!PROJECT_NAME!.sln"
set "PS1=%TEMP%\gxlib_sln.ps1"
>"!PS1!" echo $src = '!SLN_BUILD!'
>>"!PS1!" echo $dst = '!SLN_ROOT!'
>>"!PS1!" echo (Get-Content $src) -replace '(\w+\.vcxproj)', 'build/$1' ^| Set-Content $dst -Encoding UTF8
powershell -NoProfile -ExecutionPolicy Bypass -File "!PS1!"
del "!PS1!" >nul 2>&1

echo.
echo   %GREEN%プロジェクト作成完了！%RESET%
echo.
echo   フォルダ      : !PROJECT_DIR!
echo   ソリューション: !SLN_ROOT!
echo.

:: --- Visual Studio で開く ---
set "OPEN_VS=Y"
set /p "OPEN_VS=  Visual Studio で開きますか? [Y/n]: "
if /i "!OPEN_VS!"=="n" goto :project_done
echo.
echo   Visual Studio を起動しています...
start "" "!SLN_ROOT!"

:project_done
goto :done

:skip_project
echo.

:: ===================================================================
:: [5/5] 完了
:: ===================================================================
:done
echo.
echo %BOLD%%GREEN%[5/5] セットアップ完了！%RESET%
echo.
echo   %BOLD%SDK%RESET%        : !SDK_DIR!
echo   %DIM%include/ lib/ Shaders/ cmake/ template/%RESET%
echo.
echo   ------------------------------------------
echo   %BOLD%手動でプロジェクトを作る場合%RESET%
echo   ------------------------------------------
echo   template/ を好きな場所にコピーして:
echo.
echo     cmake -B build -S . -DGXLib_DIR="!SDK_DIR!\cmake"
echo.
echo   生成された .sln を Visual Studio で開いてビルド！
echo   ------------------------------------------
echo.
pause
exit /b 0
