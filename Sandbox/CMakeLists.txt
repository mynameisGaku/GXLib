# =============================================================================
# Sandbox — GXLibテストアプリケーション
#
# 【初学者向け解説】
# Sandboxは、GXLibエンジンの機能をテストするためのアプリケーションです。
# Phase 0では、色付き三角形を描画する「Hello Triangle」を実行します。
# =============================================================================

# ソースファイル
set(SANDBOX_SOURCES
    main.cpp
)

# Windows GUIアプリケーション（コンソールではなくウィンドウ）としてビルド
add_executable(Sandbox WIN32 ${SANDBOX_SOURCES})

# GXLibエンジンをリンク
target_link_libraries(Sandbox PRIVATE GXLib)

# インクルードパス
target_include_directories(Sandbox PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
)

# シェーダーファイルを出力ディレクトリにコピー
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:Sandbox>/Shaders"
    COMMENT "Copying shaders to output directory..."
)

# アセットファイルを出力ディレクトリにコピー
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:Sandbox>/Assets"
    COMMENT "Copying assets to output directory..."
)

# Visual Studioデバッガの作業ディレクトリをexeの場所に設定
# これにより、相対パス（Shaders/...）が正しく解決される
set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Sandbox>"
)
