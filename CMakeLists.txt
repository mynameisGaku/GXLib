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

# FBX SDK (optional)
set(FBX_SDK_ROOT "${CMAKE_SOURCE_DIR}/ThirdParty/FBXSDK/2020.3.9" CACHE PATH "FBX SDK root")
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
FetchContent_MakeAvailable(JoltPhysics)

# 繝・せ繝域怏蜉ｹ蛹・
enable_testing()

# 繧ｵ繝悶・繝ｭ繧ｸ繧ｧ繧ｯ繝医ｒ霑ｽ蜉
add_subdirectory(GXLib)
add_subdirectory(Sandbox)
add_subdirectory(Tests)

# Asset pipeline tools
add_subdirectory(gxconv)
add_subdirectory(gxloader)
add_subdirectory(gxpak)

# =============================================================================
# 繧ｵ繝ｳ繝励Ν繝励Ο繧ｸ繧ｧ繧ｯ繝亥・騾壹・繧ｯ繝ｭ
# =============================================================================
macro(gxlib_add_sample SAMPLE_NAME)
    add_executable(${SAMPLE_NAME} WIN32 main.cpp)
    target_link_libraries(${SAMPLE_NAME} PRIVATE GXLib)
    target_include_directories(${SAMPLE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/GXLib
    )

    # Shaders/Assets 繧貞・蜉帙ョ繧｣繝ｬ繧ｯ繝医Μ縺ｫ繧ｳ繝斐・
    add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Shaders"
            "$<TARGET_FILE_DIR:${SAMPLE_NAME}>/Shaders"
        COMMENT "Copying shaders for ${SAMPLE_NAME}..."
    )
    add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:${SAMPLE_NAME}>/Assets"
        COMMENT "Copying assets for ${SAMPLE_NAME}..."
    )

    if (FBX_SDK_FOUND)
        add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${FBX_SDK_DLL_DEBUG},${FBX_SDK_DLL_RELEASE}>"
                "$<TARGET_FILE_DIR:${SAMPLE_NAME}>"
            COMMENT "Copying FBX SDK runtime for ${SAMPLE_NAME}..."
        )
    endif()

    # VS 繝・ヰ繝・ぎ菴懈･ｭ繝・ぅ繝ｬ繧ｯ繝医Μ
    set_target_properties(${SAMPLE_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${SAMPLE_NAME}>"
    )
endmacro()

# 繧ｵ繝ｳ繝励Ν繝励Ο繧ｸ繧ｧ繧ｯ繝・
add_subdirectory(Samples/Shooting2D)
add_subdirectory(Samples/Platformer2D)
add_subdirectory(Samples/Walkthrough3D)
add_subdirectory(Samples/GUIMenuDemo)
add_subdirectory(Samples/PostEffectShowcase)
add_subdirectory(Samples/EasyHello)
add_subdirectory(Samples/DXRShowcase)

# GXModelViewer (ImGui-based 3D model viewer)
add_subdirectory(GXModelViewer)

# Sandbox繧偵せ繧ｿ繝ｼ繝医い繝・・繝励Ο繧ｸ繧ｧ繧ｯ繝医↓險ｭ螳・
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)

