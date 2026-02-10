# =============================================================================
# GXLib Framework Template
# =============================================================================

set(GXFRAMEWORK_LIB_SOURCES
    src/FrameworkApp.cpp
    src/SceneManager.cpp
    src/GameObject.cpp
    src/GameScene.cpp
    src/scenes/BootScene.cpp

    src/FrameworkApp.h
    src/Scene.h
    src/SceneManager.h
    src/GameObject.h
    src/GameScene.h
    src/scenes/BootScene.h
)

add_library(GXFrameworkLib STATIC ${GXFRAMEWORK_LIB_SOURCES})
target_link_libraries(GXFrameworkLib PUBLIC GXLib)
target_include_directories(GXFrameworkLib PUBLIC
    ${CMAKE_SOURCE_DIR}/GXLib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(GXFRAMEWORK_APP_SOURCES
    src/main.cpp
    src/scenes/SkinnedMeshScene.cpp
    src/scenes/SkinnedMeshScene.h
)

add_executable(GXFramework WIN32 ${GXFRAMEWORK_APP_SOURCES})
target_link_libraries(GXFramework PRIVATE GXFrameworkLib)
target_include_directories(GXFramework PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Shaders/Assets to output directory
add_custom_command(TARGET GXFramework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Shaders"
        "$<TARGET_FILE_DIR:GXFramework>/Shaders"
    COMMENT "Copying shaders for GXFramework..."
)
add_custom_command(TARGET GXFramework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:GXFramework>/Assets"
    COMMENT "Copying framework assets for GXFramework..."
)

# FBX SDK runtime (optional)
if (FBX_SDK_FOUND)
    add_custom_command(TARGET GXFramework POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${FBX_SDK_DLL_DEBUG},${FBX_SDK_DLL_RELEASE}>"
            "$<TARGET_FILE_DIR:GXFramework>"
        COMMENT "Copying FBX SDK runtime for GXFramework..."
    )
endif()

# VS debugger working directory
set_target_properties(GXFramework PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:GXFramework>"
)
# FBX SDK link (needed for GXLib static linkage)
if (FBX_SDK_FOUND AND MSVC AND FBX_SDK_LIB_DIR)
    target_link_libraries(GXFramework PRIVATE
        debug "${FBX_SDK_LIB_DIR}/debug/libfbxsdk.lib"
        optimized "${FBX_SDK_LIB_DIR}/release/libfbxsdk.lib"
    )
endif()
