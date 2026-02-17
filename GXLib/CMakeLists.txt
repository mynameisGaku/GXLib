# =============================================================================
# GXLib エンジンライブラリ
#
# 【初学者向け解説】
# このCMakeファイルはGXLibエンジンをスタティックライブラリ（.lib）として
# ビルドする設定です。スタティックライブラリとは、他のプログラムに
# 組み込んで使うためのコードの集まりです。
# =============================================================================

# ソースファイルを収集（ThirdPartyのヘッダーオンリーライブラリは除外）
# NOTE: *.h パターンは Visual Studio Solution Explorer でヘッダーを表示するために
# 意図的に含めています。ビルドには .cpp のみが使用されます。
file(GLOB_RECURSE GXLIB_SOURCES
    "Core/*.cpp"        "Core/*.h"
    "Graphics/*.cpp"    "Graphics/*.h"
    "Input/*.cpp"       "Input/*.h"
    "Audio/*.cpp"       "Audio/*.h"
    "GUI/*.cpp"         "GUI/*.h"
    "IO/*.cpp"          "IO/*.h"
    "Movie/*.cpp"       "Movie/*.h"
    "Math/*.cpp"        "Math/*.h"
    "Physics/*.cpp"     "Physics/*.h"
    "Compat/*.cpp"      "Compat/*.h"
    "pch.cpp"           "pch.h"
)

# LZ4 (ThirdParty — single-file compression library)
list(APPEND GXLIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lz4.c"
)

# スタティックライブラリとしてビルド
add_library(GXLib STATIC ${GXLIB_SOURCES})

# インクルードパス設定
target_include_directories(GXLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
)

# プリコンパイルドヘッダー設定
target_precompile_headers(GXLib PRIVATE pch.h)

if (MSVC)
    target_compile_options(GXLib PRIVATE /FS)
endif()

# lz4.c はPCHを使用しない（Cファイルのため）
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lz4.c"
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        LANGUAGE C
)

# リンクするライブラリ
target_link_libraries(GXLib PUBLIC
    d3d12.lib
    dxgi.lib
    dxcompiler.lib
    dxguid.lib
    xinput.lib
    xaudio2.lib
    ole32.lib
    dwrite.lib
    d2d1.lib
    windowscodecs.lib
    bcrypt.lib          # AES-256, SHA-256
    ws2_32.lib          # Winsock (TCP/UDP)
    winhttp.lib         # HTTP, WebSocket
    mfplat.lib          # Media Foundation
    mfreadwrite.lib     # MF Source Reader
    mf.lib              # MF Core
    mfuuid.lib          # MF GUIDs
    Jolt                # Jolt Physics
)

# FBX SDK (optional)
if (FBX_SDK_FOUND)
    target_include_directories(GXLib PRIVATE
        ${FBX_SDK_ROOT}/include
    )
    target_compile_definitions(GXLib PRIVATE FBXSDK_SHARED)

    if (MSVC AND FBX_SDK_LIB_DIR)
        target_link_libraries(GXLib PUBLIC
            debug "${FBX_SDK_LIB_DIR}/debug/libfbxsdk.lib"
            optimized "${FBX_SDK_LIB_DIR}/release/libfbxsdk.lib"
        )
    endif()
endif()

# Jolt Physics include path
target_include_directories(GXLib PUBLIC
    ${joltphysics_SOURCE_DIR}/..
)

# gxformat headers + gxloader library (asset pipeline)
target_include_directories(GXLib PUBLIC ${CMAKE_SOURCE_DIR}/gxformat)
target_link_libraries(GXLib PUBLIC gxloader)
