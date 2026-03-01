# =============================================================================
# GXLib エンジンライブラリ（モジュール分割版）
#
# 各サブシステムを独立したスタティックライブラリとしてビルドし、
# アンブレラターゲット GXLib が全てをリンクする。
# 既存コードは target_link_libraries(MyApp GXLib) で変更不要。
# =============================================================================

# 全モジュール共通のインクルードパスとコンパイルオプション
set(GXLIB_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
)

# ---------------------------------------------------------------------------
# ヘルパーマクロ: サブライブラリの共通設定を適用する
# pch_header: モジュール固有のプリコンパイルドヘッダーファイル
#   pch_common.h   — Windows + STL + DirectXMath（Graphics非依存モジュール用）
#   pch_graphics.h — + D3D12/DXGI/DirectWrite/D2D1（Graphics/GUI用）
#   pch_audio.h    — + XAudio2/X3DAudio（Audio用）
#   pch.h          — 全部入り（Compat/GXアンブレラ用・後方互換）
# ---------------------------------------------------------------------------
macro(gxlib_setup_module target pch_header)
    target_include_directories(${target} PUBLIC ${GXLIB_INCLUDE_DIRS})
    target_precompile_headers(${target} PRIVATE ${pch_header})
    if (MSVC)
        target_compile_options(${target} PRIVATE /FS)
    endif()
endmacro()

# =============================================================================
# GXLib_Foundation — Math, Collision, Spatial
# 依存: なし
# =============================================================================
file(GLOB_RECURSE FOUNDATION_SOURCES "Math/*.cpp" "Math/*.h")
add_library(GXLib_Foundation STATIC ${FOUNDATION_SOURCES})
gxlib_setup_module(GXLib_Foundation pch_common.h)
set_target_properties(GXLib_Foundation PROPERTIES FOLDER "Engine")

# =============================================================================
# GXLib_Core — Application, Window, Timer, Logger, Allocators
# 依存: GXLib_Foundation
# =============================================================================
file(GLOB_RECURSE CORE_SOURCES "Core/*.cpp" "Core/*.h")
# Scene/ は別モジュールなので除外
list(FILTER CORE_SOURCES EXCLUDE REGEX "Core/Scene/")
add_library(GXLib_Core STATIC ${CORE_SOURCES})
gxlib_setup_module(GXLib_Core pch_common.h)
set_target_properties(GXLib_Core PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Core PUBLIC GXLib_Foundation)

# =============================================================================
# GXLib_Input — Keyboard, Mouse, Gamepad, InputManager, ActionMapping
# 依存: GXLib_Core
# =============================================================================
file(GLOB_RECURSE INPUT_SOURCES "Input/*.cpp" "Input/*.h")
add_library(GXLib_Input STATIC ${INPUT_SOURCES})
gxlib_setup_module(GXLib_Input pch_common.h)
set_target_properties(GXLib_Input PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Input PUBLIC GXLib_Core)
target_link_libraries(GXLib_Input PUBLIC xinput.lib)

# =============================================================================
# GXLib_Audio — AudioDevice, Sound, SoundPlayer, AudioManager, AudioMixer
# 依存: GXLib_Core
# =============================================================================
file(GLOB_RECURSE AUDIO_SOURCES "Audio/*.cpp" "Audio/*.h")
add_library(GXLib_Audio STATIC ${AUDIO_SOURCES})
gxlib_setup_module(GXLib_Audio pch_audio.h)
set_target_properties(GXLib_Audio PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Audio PUBLIC GXLib_Core)
target_link_libraries(GXLib_Audio PUBLIC xaudio2.lib)

# =============================================================================
# GXLib_IO — FileSystem, Archive, AsyncLoader, Network, MoviePlayer
# 依存: GXLib_Core
# =============================================================================
file(GLOB_RECURSE IO_SOURCES "IO/*.cpp" "IO/*.h" "Movie/*.cpp" "Movie/*.h")
# LZ4 (ThirdParty — single-file compression library)
list(APPEND IO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lz4.c")
add_library(GXLib_IO STATIC ${IO_SOURCES})
gxlib_setup_module(GXLib_IO pch_common.h)
set_target_properties(GXLib_IO PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_IO PUBLIC GXLib_Core gxloader)
target_include_directories(GXLib_IO PUBLIC ${CMAKE_SOURCE_DIR}/gxformat)
target_link_libraries(GXLib_IO PUBLIC
    bcrypt.lib ws2_32.lib winhttp.lib
    mfplat.lib mfreadwrite.lib mf.lib mfuuid.lib
)
# lz4.c は C ファイルのため PCH スキップ
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lz4.c"
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON LANGUAGE C
)

# =============================================================================
# GXLib_Physics — PhysicsWorld2D/3D
# 依存: GXLib_Foundation, Jolt
# =============================================================================
file(GLOB_RECURSE PHYSICS_SOURCES "Physics/*.cpp" "Physics/*.h")
add_library(GXLib_Physics STATIC ${PHYSICS_SOURCES})
gxlib_setup_module(GXLib_Physics pch_common.h)
set_target_properties(GXLib_Physics PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Physics PUBLIC GXLib_Foundation Jolt)
target_include_directories(GXLib_Physics PUBLIC ${joltphysics_SOURCE_DIR}/..)
target_include_directories(GXLib_Physics PRIVATE ${CMAKE_SOURCE_DIR}/extern)

# =============================================================================
# GXLib_AI — NavMesh, NavAgent
# 依存: GXLib_Foundation
# =============================================================================
file(GLOB_RECURSE AI_SOURCES "AI/*.cpp" "AI/*.h")
add_library(GXLib_AI STATIC ${AI_SOURCES})
gxlib_setup_module(GXLib_AI pch_common.h)
set_target_properties(GXLib_AI PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_AI PUBLIC GXLib_Foundation)

# =============================================================================
# GXLib_Scene — Entity, Component, Scene, SceneSerializer
# 依存: GXLib_Foundation
# =============================================================================
file(GLOB_RECURSE SCENE_SOURCES "Core/Scene/*.cpp" "Core/Scene/*.h")
add_library(GXLib_Scene STATIC ${SCENE_SOURCES})
gxlib_setup_module(GXLib_Scene pch_common.h)
set_target_properties(GXLib_Scene PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Scene PUBLIC GXLib_Foundation)
target_include_directories(GXLib_Scene PRIVATE ${CMAKE_SOURCE_DIR}/extern)

# =============================================================================
# GXLib_ECS — Data-oriented ECS (World, Archetype, ComponentStorage, Query, System)
# 依存: GXLib_Core, GXLib_Scene
# =============================================================================
file(GLOB_RECURSE ECS_SOURCES "ECS/*.cpp" "ECS/*.h")
add_library(GXLib_ECS STATIC ${ECS_SOURCES})
gxlib_setup_module(GXLib_ECS pch_common.h)
set_target_properties(GXLib_ECS PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_ECS PUBLIC GXLib_Core GXLib_Scene)

# =============================================================================
# GXLib_Graphics — Device, Pipeline, Resource, Rendering, 3D, PostEffect,
#                  Layer, RayTracing
# 依存: GXLib_Core, d3d12, dxgi, gxloader
# =============================================================================
file(GLOB_RECURSE GRAPHICS_SOURCES "Graphics/*.cpp" "Graphics/*.h")
add_library(GXLib_Graphics STATIC ${GRAPHICS_SOURCES})
gxlib_setup_module(GXLib_Graphics pch_graphics.h)
set_target_properties(GXLib_Graphics PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Graphics PUBLIC
    GXLib_Core gxloader
    d3d12.lib dxgi.lib dxcompiler.lib dxguid.lib
    ole32.lib dwrite.lib d2d1.lib windowscodecs.lib
)
target_include_directories(GXLib_Graphics PUBLIC ${CMAKE_SOURCE_DIR}/gxformat)

# FBX SDK (optional)
if (FBX_SDK_FOUND)
    target_include_directories(GXLib_Graphics PRIVATE ${FBX_SDK_ROOT}/include)
    target_compile_definitions(GXLib_Graphics PRIVATE FBXSDK_SHARED)
    if (MSVC AND FBX_SDK_LIB_DIR)
        target_link_libraries(GXLib_Graphics PUBLIC
            debug "${FBX_SDK_LIB_DIR}/debug/libfbxsdk.lib"
            optimized "${FBX_SDK_LIB_DIR}/release/libfbxsdk.lib"
        )
    endif()
endif()

# =============================================================================
# GXLib_GUI — Widget, UIContext, UIRenderer, StyleSheet, Widgets/
# 依存: GXLib_Graphics, GXLib_Input
# =============================================================================
file(GLOB_RECURSE GUI_SOURCES "GUI/*.cpp" "GUI/*.h")
add_library(GXLib_GUI STATIC ${GUI_SOURCES})
gxlib_setup_module(GXLib_GUI pch_graphics.h)
set_target_properties(GXLib_GUI PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_GUI PUBLIC GXLib_Graphics GXLib_Input)

# =============================================================================
# GXLib_Editor — Gizmo, EntityPicker, ShaderGraph, SceneViewport, TerrainSculptor, etc.
# 依存: GXLib_Graphics, GXLib_Scene
# =============================================================================
file(GLOB_RECURSE EDITOR_SOURCES "Editor/*.cpp" "Editor/*.h")
add_library(GXLib_Editor STATIC ${EDITOR_SOURCES})
gxlib_setup_module(GXLib_Editor pch_graphics.h)
set_target_properties(GXLib_Editor PROPERTIES FOLDER "Engine")
target_link_libraries(GXLib_Editor PUBLIC GXLib_Graphics GXLib_Scene)

# =============================================================================
# GXLib_Script — ScriptEngine, ScriptBindings [optional: GX_ENABLE_LUA]
# 依存: GXLib_Core, lua, sol2
# =============================================================================
if (GX_ENABLE_LUA)
    file(GLOB_RECURSE SCRIPT_SOURCES "Script/*.cpp" "Script/*.h")
    add_library(GXLib_Script STATIC ${SCRIPT_SOURCES})
    gxlib_setup_module(GXLib_Script pch_common.h)
    set_target_properties(GXLib_Script PROPERTIES FOLDER "Engine")
    target_link_libraries(GXLib_Script PUBLIC GXLib_Core lua_static)
    target_include_directories(GXLib_Script PUBLIC ${sol2_SOURCE_DIR}/include)
    target_compile_definitions(GXLib_Script PUBLIC GX_ENABLE_LUA)

    # sol2 ヘッダーは PCH と競合するため個別にスキップ
    set_source_files_properties(${SCRIPT_SOURCES} PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
    )
endif()

# =============================================================================
# GXLib — アンブレラターゲット（Compat/ + 全サブライブラリ統合）
#
# target_link_libraries(MyApp GXLib) で全機能を利用可能（後方互換）。
# =============================================================================
file(GLOB_RECURSE COMPAT_SOURCES "Compat/*.cpp" "Compat/*.h" "GX/*.cpp" "GX/*.h")
list(APPEND COMPAT_SOURCES "pch.cpp" "pch.h")
add_library(GXLib STATIC ${COMPAT_SOURCES})
gxlib_setup_module(GXLib pch.h)
set_target_properties(GXLib PROPERTIES FOLDER "Engine")

# 全サブライブラリをリンク（PUBLIC で依存先も連鎖）
target_link_libraries(GXLib PUBLIC
    GXLib_Foundation
    GXLib_Core
    GXLib_Input
    GXLib_Audio
    GXLib_IO
    GXLib_Physics
    GXLib_AI
    GXLib_Scene
    GXLib_ECS
    GXLib_Graphics
    GXLib_GUI
    GXLib_Editor
)

if (GX_ENABLE_LUA)
    target_link_libraries(GXLib PUBLIC GXLib_Script)
endif()

# Jolt Physics include path (umbrella から公開)
target_include_directories(GXLib PUBLIC ${joltphysics_SOURCE_DIR}/..)

# =============================================================================
# install() ルール — cmake --install で SDK を生成
# =============================================================================

# ヘッダーインストール（ディレクトリ構造保持）
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/AI
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/Compat
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/Editor
    ${CMAKE_CURRENT_SOURCE_DIR}/Graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/GX
    ${CMAKE_CURRENT_SOURCE_DIR}/Input
    ${CMAKE_CURRENT_SOURCE_DIR}/IO
    ${CMAKE_CURRENT_SOURCE_DIR}/Math
    ${CMAKE_CURRENT_SOURCE_DIR}/Movie
    ${CMAKE_CURRENT_SOURCE_DIR}/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/Script
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
    DESTINATION include/GXLib
    FILES_MATCHING PATTERN "*.h"
)

# ルート直下ヘッダー (GXLib.h, pch*.h)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/GXLib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pch_common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pch_graphics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pch_audio.h
    DESTINATION include/GXLib
)

# ライブラリインストール（Debug/Release を $<CONFIG> で分岐）
install(TARGETS
    GXLib GXLib_Foundation GXLib_Core GXLib_Input GXLib_Audio
    GXLib_IO GXLib_Physics GXLib_AI GXLib_Scene GXLib_ECS
    GXLib_Graphics GXLib_GUI GXLib_Editor
    ARCHIVE DESTINATION lib/$<CONFIG>
)

if (GX_ENABLE_LUA)
    install(TARGETS GXLib_Script lua_static ARCHIVE DESTINATION lib/$<CONFIG>)
endif()
