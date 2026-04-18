# =============================================================================
# Tests/isolation/ai_foundation_only — ADR-0018 E1 link-isolation test
# =============================================================================
# Links ONLY GXLib_AI (+ its declared dep chain: Foundation + Container).
# Does NOT link GXLib_Graphics. If AI/*.cpp regains a Graphics include,
# this target's link step fails — the regression is caught at build time.
# =============================================================================

add_executable(gxlib_ai_foundation_only_test main.cpp)
target_link_libraries(gxlib_ai_foundation_only_test PRIVATE
    GXLib_AI
    GXLib_Core   # for Logger (used internally by NavMesh.cpp)
)
target_include_directories(gxlib_ai_foundation_only_test PRIVATE
    ${CMAKE_SOURCE_DIR}/GXLib
)
set_target_properties(gxlib_ai_foundation_only_test PROPERTIES
    FOLDER "Tests/Isolation"
)

# Run as part of ctest so CI catches regressions.
add_test(NAME ai_foundation_only_isolation
         COMMAND gxlib_ai_foundation_only_test)
