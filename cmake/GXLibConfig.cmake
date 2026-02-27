# =============================================================================
# GXLibConfig.cmake — find_package(GXLib) 用パッケージ設定
#
# 使い方:
#   cmake -B build -S . -DGXLib_DIR=path/to/GXLib-SDK/cmake
#   # CMakeLists.txt で:
#   find_package(GXLib REQUIRED)
#   target_link_libraries(MyApp PRIVATE GXLib::GXLib)
# =============================================================================

cmake_minimum_required(VERSION 3.24)

# SDK ルートディレクトリを算出（cmake/ の親ディレクトリ）
get_filename_component(GXLIB_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# ライブラリ一覧（リンク順序: 依存される側が後ろ）
set(_GXLIB_LIBS
    GXLib
    GXLib_GUI
    GXLib_Graphics
    GXLib_IO
    GXLib_Input
    GXLib_Audio
    GXLib_Physics
    GXLib_AI
    GXLib_Scene
    GXLib_Core
    GXLib_Foundation
    gxloader
    Jolt
)

# 各ライブラリの IMPORTED ターゲットを定義
foreach(_lib ${_GXLIB_LIBS})
    if (NOT TARGET GXLib::${_lib})
        add_library(GXLib::${_lib} STATIC IMPORTED)

        # Debug / Release それぞれの .lib パスを設定
        set_target_properties(GXLib::${_lib} PROPERTIES
            IMPORTED_LOCATION_DEBUG   "${GXLIB_SDK_ROOT}/lib/Debug/${_lib}.lib"
            IMPORTED_LOCATION_RELEASE "${GXLIB_SDK_ROOT}/lib/Release/${_lib}.lib"
            IMPORTED_CONFIGURATIONS   "Debug;Release"
        )

        # RelWithDebInfo / MinSizeRel は Release にフォールバック
        set_target_properties(GXLib::${_lib} PROPERTIES
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL     Release
        )
    endif()
endforeach()

# GXLib_Script / lua_static（オプション: Lua 有効時のみ存在）
if (EXISTS "${GXLIB_SDK_ROOT}/lib/Debug/GXLib_Script.lib" OR EXISTS "${GXLIB_SDK_ROOT}/lib/Release/GXLib_Script.lib")
    foreach(_lib GXLib_Script lua_static)
        if (NOT TARGET GXLib::${_lib})
            add_library(GXLib::${_lib} STATIC IMPORTED)
            set_target_properties(GXLib::${_lib} PROPERTIES
                IMPORTED_LOCATION_DEBUG   "${GXLIB_SDK_ROOT}/lib/Debug/${_lib}.lib"
                IMPORTED_LOCATION_RELEASE "${GXLIB_SDK_ROOT}/lib/Release/${_lib}.lib"
                IMPORTED_CONFIGURATIONS   "Debug;Release"
                MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
                MAP_IMPORTED_CONFIG_MINSIZEREL     Release
            )
        endif()
    endforeach()
    set(GXLIB_HAS_LUA TRUE)
else()
    set(GXLIB_HAS_LUA FALSE)
endif()

# インクルードパス設定（GXLib::GXLib に集約）
set_target_properties(GXLib::GXLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GXLIB_SDK_ROOT}/include/GXLib;${GXLIB_SDK_ROOT}/include"
)

# システムライブラリ + 内部ライブラリの依存関係を GXLib::GXLib に設定
set(_GXLIB_LINK_LIBS
    GXLib::GXLib_GUI
    GXLib::GXLib_Graphics
    GXLib::GXLib_IO
    GXLib::GXLib_Input
    GXLib::GXLib_Audio
    GXLib::GXLib_Physics
    GXLib::GXLib_AI
    GXLib::GXLib_Scene
    GXLib::GXLib_Core
    GXLib::GXLib_Foundation
    GXLib::gxloader
    GXLib::Jolt
    # DirectX 12
    d3d12.lib
    dxgi.lib
    dxcompiler.lib
    dxguid.lib
    # Windows / COM
    ole32.lib
    # DirectWrite / Direct2D
    dwrite.lib
    d2d1.lib
    windowscodecs.lib
    # Input
    xinput.lib
    # Audio
    xaudio2.lib
    # Crypto / Network
    bcrypt.lib
    ws2_32.lib
    winhttp.lib
    # Media Foundation
    mfplat.lib
    mfreadwrite.lib
    mf.lib
    mfuuid.lib
)

if (GXLIB_HAS_LUA)
    list(APPEND _GXLIB_LINK_LIBS GXLib::GXLib_Script GXLib::lua_static)
endif()

set_target_properties(GXLib::GXLib PROPERTIES
    INTERFACE_LINK_LIBRARIES "${_GXLIB_LINK_LIBS}"
)

# パッケージ検出完了メッセージ
set(GXLib_FOUND TRUE)
message(STATUS "Found GXLib SDK: ${GXLIB_SDK_ROOT}")
