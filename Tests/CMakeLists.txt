# =============================================================================
# GXLib Tests — Google Test
# =============================================================================

# Google Test (FetchContent)
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# テストソース
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp
    test_Allocator.cpp
    # Phase 37: テスト強化
    test_Spline.cpp
    test_Transform3D.cpp
    test_Entity.cpp
    test_SceneSerializer.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_Animation.cpp
    test_StyleSheet.cpp
    test_FileSystem.cpp
    # Phase 41: テスト拡充
    test_Transform2D.cpp
    test_Camera2D.cpp
    test_Camera3D.cpp
    test_Light.cpp
    test_Material.cpp
    test_XMLParser.cpp
    test_Tilemap.cpp
    test_IK.cpp
    test_AnimatorStateMachine.cpp
    test_BlendSystem.cpp
    # Phase 5: DI + テスタビリティ
    test_ServiceLocator.cpp
)

add_executable(GXLibTests ${TEST_SOURCES})
target_link_libraries(GXLibTests PRIVATE GXLib GTest::gtest GTest::gtest_main)
target_include_directories(GXLibTests PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
    ${CMAKE_SOURCE_DIR}
)

# CTest 連携
include(GoogleTest)
gtest_discover_tests(GXLibTests)
