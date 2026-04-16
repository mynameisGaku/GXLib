# =============================================================================
# GXLib スタータープロジェクト
#
# 使い方:
#   1. このフォルダを好きな場所にコピー
#   2. 下の project(...) 行のプロジェクト名を好きな名前に変更
#   3. GXLib_DIR を SDK の cmake/ フォルダに指定してビルド:
#      cmake -B build -S . -DGXLib_DIR=path/to/GXLib-SDK/cmake
#      cmake --build build --config Debug
#   4. main.cpp を編集してゲームを作ろう！
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(MyGame LANGUAGES CXX)   # ← プロジェクト名をここで変更

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/utf-8)
endif()

# In-tree build (from GXLib repo root) or standalone (SDK install) detection
if (NOT TARGET GXLib::GXLib)
    find_package(GXLib REQUIRED)
endif()

add_executable(${PROJECT_NAME} WIN32 main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GXLib::GXLib)

# Shaders を実行ファイルの隣にコピー
if (DEFINED GXLib_DIR)
    get_filename_component(_SHADERS_ROOT "${GXLib_DIR}/.." ABSOLUTE)
    set(_SHADERS_SRC "${_SHADERS_ROOT}/Shaders")
else()
    set(_SHADERS_SRC "${CMAKE_SOURCE_DIR}/../Shaders")  # in-tree fallback
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_SHADERS_SRC}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders"
    COMMENT "Copying Shaders to output directory"
)

# DXC ランタイム DLL を隣にコピー (起動時 dll-not-found 防止)
# In-tree の場合は root CMakeLists から関数が継承される。
# Standalone の場合は GXLibConfig.cmake 経由で取得 (将来追加)。
if (COMMAND gxlib_copy_runtime_dlls)
    gxlib_copy_runtime_dlls(${PROJECT_NAME})
endif()

# デバッガの作業ディレクトリ（Visual Studio用）
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# スタートアッププロジェクトに設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
