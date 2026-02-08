# =============================================================================
# GXLib エンジンライブラリ
#
# 【初学者向け解説】
# このCMakeファイルはGXLibエンジンをスタティックライブラリ（.lib）として
# ビルドする設定です。スタティックライブラリとは、他のプログラムに
# 組み込んで使うためのコードの集まりです。
# =============================================================================

# ソースファイルを収集（ThirdPartyのヘッダーオンリーライブラリは除外）
file(GLOB_RECURSE GXLIB_SOURCES
    "Core/*.cpp"        "Core/*.h"
    "Graphics/*.cpp"    "Graphics/*.h"
    "Input/*.cpp"       "Input/*.h"
    "Audio/*.cpp"       "Audio/*.h"
    "pch.cpp"           "pch.h"
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

# リンクするDirectX 12関連ライブラリ
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
)
