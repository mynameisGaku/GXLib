# =============================================================================
# GXLib スタータープロジェクト
#
# 使い方:
#   1. このフォルダを好きな場所にコピー
#   2. GXLib_DIR を SDK の cmake/ フォルダに指定してビルド:
#      cmake -B build -S . -DGXLib_DIR=path/to/GXLib-SDK/cmake
#      cmake --build build --config Debug
#   3. main.cpp を編集してゲームを作ろう！
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/utf-8)
endif()

# GXLib SDK を検索
find_package(GXLib REQUIRED)

add_executable(MyGame WIN32 main.cpp)
target_link_libraries(MyGame PRIVATE GXLib::GXLib)

# Shaders を実行ファイルの隣にコピー
# GXLib_DIR は cmake/ を指しているので、その親の Shaders/ をコピー
get_filename_component(_SDK_ROOT "${GXLib_DIR}/.." ABSOLUTE)
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_SDK_ROOT}/Shaders"
        "$<TARGET_FILE_DIR:MyGame>/Shaders"
    COMMENT "Copying Shaders to output directory"
)

# デバッガの作業ディレクトリ（Visual Studio用）
set_target_properties(MyGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MyGame>"
)
