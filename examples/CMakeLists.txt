# =============================================================================
# GXLib Examples — Layer 1.5 sample applications (per ADR-0017)
#
# Two categories:
#   Basic (GX_BUILD_EXAMPLES, default ON):
#     01-hello-sprite, 02-hello-sound, 03-hello-input, 04-hello-3d,
#     05-custom-postfx, 07-hello-ecs, 09-hello-animation
#   Advanced (GX_BUILD_ADVANCED_EXAMPLES, default OFF):
#     06-custom-shader-model, 08-hello-physics, 10-custom-asset-type,
#     11-custom-audio-dsp, 12-hello-gui, 13-custom-widget
#
# The Advanced samples demonstrate Layer 2 extension patterns but use
# APIs that require engine-side verification. They are included as
# reference/spec code; enable GX_BUILD_ADVANCED_EXAMPLES to try building them
# and you may need to adjust some API names to match the actual engine.
# =============================================================================

option(GX_BUILD_ADVANCED_EXAMPLES "Build L2 advanced samples (may need API tweaks)" OFF)

# In-tree build detection: provide GXLib::GXLib alias
if (TARGET GXLib AND NOT TARGET GXLib::GXLib)
    add_library(GXLib::GXLib ALIAS GXLib)
endif()

# --- Basic samples (known-working, build by default) -------------------------
add_subdirectory(01-hello-sprite)
add_subdirectory(02-hello-sound)
add_subdirectory(03-hello-input)
add_subdirectory(04-hello-3d)
add_subdirectory(05-custom-postfx)
add_subdirectory(07-hello-ecs)
add_subdirectory(09-hello-animation)

# --- Advanced samples (L2 showcases, opt-in) ---------------------------------
if (GX_BUILD_ADVANCED_EXAMPLES)
    add_subdirectory(06-custom-shader-model)
    add_subdirectory(08-hello-physics)
    add_subdirectory(10-custom-asset-type)
    add_subdirectory(11-custom-audio-dsp)
    add_subdirectory(12-hello-gui)
    add_subdirectory(13-custom-widget)
    add_subdirectory(14-hello-network)
    add_subdirectory(15-hello-physics3d)
endif()

# VS solution folder grouping (親ディレクトリからは set_target_properties のみ許可)
# DXC DLL コピーは各サンプルの CMakeLists.txt 内で gxlib_copy_runtime_dlls() を呼ぶ
if (TARGET GXLib)
    foreach(n RANGE 1 15)
        if (n LESS 10)
            set(_target "gxlib_example_0${n}")
        else()
            set(_target "gxlib_example_${n}")
        endif()
        if (TARGET ${_target})
            set_target_properties(${_target} PROPERTIES FOLDER "Examples")
        endif()
    endforeach()
endif()
