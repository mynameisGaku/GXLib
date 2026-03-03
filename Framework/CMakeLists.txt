# =============================================================================
# GXLib Framework — 高レベルゲームフレームワーク
#
# GXLib SDK を利用するスタンドアロンプロジェクト。
# GXLib.sln には含まれず、独立してビルドする。
#
# ビルド手順:
#   1. GXLib SDK をビルド・インストール:
#      cd path/to/GXLib
#      cmake -B build -S . -DGX_SDK_ONLY=ON
#      cmake --build build --config Debug
#      cmake --install build --prefix GXLib-SDK --config Debug
#
#   2. Framework をビルド:
#      cd Framework
#      cmake -B build -S . -DGXLib_DIR=../GXLib-SDK/cmake
#      cmake --build build --config Debug
#
#   3. 実行:
#      build/Debug/Framework.exe
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(Framework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/utf-8)
endif()

# GXLib SDK を検索
find_package(GXLib REQUIRED)

add_executable(Framework WIN32
    src/main.cpp
    src/GameFramework.cpp
    src/GameFramework.h
)
target_link_libraries(Framework PRIVATE GXLib::GXLib)
target_include_directories(Framework PRIVATE src)

# Shaders を実行ファイルの隣にコピー
get_filename_component(_SDK_ROOT "${GXLib_DIR}/.." ABSOLUTE)
add_custom_command(TARGET Framework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_SDK_ROOT}/Shaders"
        "$<TARGET_FILE_DIR:Framework>/Shaders"
    COMMENT "Copying Shaders to output directory"
)

# デバッガの作業ディレクトリ（Visual Studio用）
set_target_properties(Framework PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Framework>"
)

# スタートアッププロジェクトに設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT Framework)
