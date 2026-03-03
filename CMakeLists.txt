# =============================================================================
# GXLib 窶・DirectX 12 繧ｲ繝ｼ繝繝輔Ξ繝ｼ繝繝ｯ繝ｼ繧ｯ
#
# 縲仙・蟄ｦ閠・髄縺題ｧ｣隱ｬ縲・
# CMake縺ｯC++繝励Ο繧ｸ繧ｧ繧ｯ繝医・繝薙Ν繝峨す繧ｹ繝・Β縺ｧ縺吶・
# 縺薙・繝輔ぃ繧､繝ｫ縺後・繝ｭ繧ｸ繧ｧ繧ｯ繝亥・菴薙・險ｭ螳壹ｒ螳夂ｾｩ縺励・
# 繧ｵ繝悶ョ繧｣繝ｬ繧ｯ繝医Μ・・XLib縲ヾandbox・峨ｒ縺ｾ縺ｨ繧√※縺・∪縺吶・
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(GXLib VERSION 0.1.0 LANGUAGES CXX)

# Visual Studio ソリューションフォルダを有効化（プロジェクトをグループ整理）
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# FBX SDK (optional)
set(FBX_SDK_ROOT "${CMAKE_SOURCE_DIR}/extern/ThirdParty/FBXSDK/2020.3.9" CACHE PATH "FBX SDK root")
set(FBX_SDK_FOUND FALSE)
if (EXISTS "${FBX_SDK_ROOT}/include/fbxsdk.h")
    set(FBX_SDK_FOUND TRUE)
    if (WIN32)
        set(FBX_SDK_LIB_DIR "${FBX_SDK_ROOT}/lib/x64")
        set(FBX_SDK_DLL_DEBUG "${FBX_SDK_LIB_DIR}/debug/libfbxsdk.dll")
        set(FBX_SDK_DLL_RELEASE "${FBX_SDK_LIB_DIR}/release/libfbxsdk.dll")
    endif()
endif()

# C++20繧剃ｽｿ逕ｨ
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Visual Studio: UTF-8繧ｽ繝ｼ繧ｹ繝輔ぃ繧､繝ｫ蟇ｾ蠢懶ｼ域律譛ｬ隱槭さ繝｡繝ｳ繝亥ｿ・茨ｼ・
add_compile_options(/utf-8)
if (MSVC)
    add_compile_options(/FS)
endif()

# Jolt Physics (FetchContent)
include(FetchContent)
FetchContent_Declare(JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.3.0
    SOURCE_SUBDIR  Build
)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(JoltPhysics)
set_target_properties(Jolt PROPERTIES FOLDER "ThirdParty")

# Lua 5.4 (FetchContent)
option(GX_ENABLE_LUA "Enable Lua scripting support" ON)
if (GX_ENABLE_LUA)
    FetchContent_Declare(lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG        v5.4.7
    )
    FetchContent_MakeAvailable(lua)

    # Lua はCMakeプロジェクトではないので手動でライブラリを構築
    file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
    # lua.c と luac.c は実行ファイルのエントリポイントなので除外
    list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
    list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")

    add_library(lua_static STATIC ${LUA_SOURCES})
    target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR})
    set_target_properties(lua_static PROPERTIES LANGUAGE C FOLDER "ThirdParty")
    set_source_files_properties(${LUA_SOURCES} PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        LANGUAGE C
    )
    if (MSVC)
        target_compile_options(lua_static PRIVATE /FS)
    endif()

    # sol2 (header-only)
    set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG        v3.3.1
    )
    FetchContent_MakeAvailable(sol2)
endif()

# SDK ライブラリのみビルド (ツール・テスト・サンプルを除外)
option(GX_SDK_ONLY "Build only SDK libraries (skip tools, tests, samples)" OFF)

# SDK に必要なライブラリ
add_subdirectory(GXLib)
add_subdirectory(extern/gxloader)

# =============================================================================
# Doxygen ドキュメント生成 (optional)
# =============================================================================
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    set_target_properties(docs PROPERTIES FOLDER "Tools")
endif()

if (NOT GX_SDK_ONLY)
    enable_testing()
    add_subdirectory(Tests)
    add_subdirectory(tools/gxconv)
    add_subdirectory(tools/gxpak)
    add_subdirectory(GXModelViewer)

    # スタートアッププロジェクト
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GXLibTests)

    # ソリューションフォルダ整理
    set_target_properties(gxconv         PROPERTIES FOLDER "Tools")
    set_target_properties(gxloader       PROPERTIES FOLDER "Tools")
    set_target_properties(gxpak          PROPERTIES FOLDER "Tools")
    set_target_properties(GXModelViewer  PROPERTIES FOLDER "Tools")
    set_target_properties(GXLibTests     PROPERTIES FOLDER "Tests")
    if (TARGET gtest)
        set_target_properties(gtest        PROPERTIES FOLDER "ThirdParty")
    endif()
    if (TARGET gtest_main)
        set_target_properties(gtest_main   PROPERTIES FOLDER "ThirdParty")
    endif()
    if (TARGET gmock)
        set_target_properties(gmock        PROPERTIES FOLDER "ThirdParty")
    endif()
    if (TARGET gmock_main)
        set_target_properties(gmock_main   PROPERTIES FOLDER "ThirdParty")
    endif()
endif()

# =============================================================================
# install() ルール — cmake --install で SDK パッケージを生成
#
# 使い方:
#   cmake --build build --config Release
#   cmake --install build --prefix GXLib-SDK --config Release
# =============================================================================

# gxloader, Jolt ライブラリ
install(TARGETS gxloader Jolt ARCHIVE DESTINATION lib/$<CONFIG>)

# gxformat ヘッダー
install(DIRECTORY ${CMAKE_SOURCE_DIR}/extern/gxformat/ DESTINATION include/gxformat FILES_MATCHING PATTERN "*.h")

# gxloader ヘッダー
install(DIRECTORY ${CMAKE_SOURCE_DIR}/extern/gxloader/ DESTINATION include/gxloader FILES_MATCHING PATTERN "*.h")

# シェーダー（実行時必須）
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders/ DESTINATION Shaders FILES_MATCHING PATTERN "*.hlsl" PATTERN "*.hlsli")

# CMake パッケージ設定（find_package(GXLib) 用）
install(FILES ${CMAKE_SOURCE_DIR}/cmake/GXLibConfig.cmake DESTINATION cmake)

# スターターテンプレート
install(DIRECTORY ${CMAKE_SOURCE_DIR}/template/ DESTINATION template)

