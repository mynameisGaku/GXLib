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
)

add_executable(GXLibTests ${TEST_SOURCES})
target_link_libraries(GXLibTests PRIVATE GXLib GTest::gtest GTest::gtest_main)
target_include_directories(GXLibTests PRIVATE ${CMAKE_SOURCE_DIR}/GXLib)

# CTest 連携
include(GoogleTest)
gtest_discover_tests(GXLibTests)
