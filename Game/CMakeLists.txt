# Game — ここに自分のゲームを作ろう！
#
# main.cpp を編集してゲームを作成してください。
# 画像や音声ファイルは Assets/ フォルダに置くと自動でコピーされます。

add_executable(Game WIN32 main.cpp)

target_link_libraries(Game PRIVATE GXLib)

target_include_directories(Game PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
)

# Shaders / Assets を実行ファイルの隣にコピー
add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:Game>/Shaders"
)
add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:Game>/Assets"
)

# デバッガの作業ディレクトリ（Visual Studio用）
set_target_properties(Game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>"
)
