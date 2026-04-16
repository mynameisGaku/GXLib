cmake_minimum_required(VERSION 3.24)
project(gxlib_example_06 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/utf-8)
endif()
if (NOT TARGET GXLib::GXLib)
    find_package(GXLib REQUIRED)
endif()
add_executable(gxlib_example_06 WIN32 main.cpp)
target_link_libraries(gxlib_example_06 PRIVATE GXLib::GXLib)
if (DEFINED GXLib_DIR)
    get_filename_component(_SHADERS_ROOT "${GXLib_DIR}/.." ABSOLUTE)
    set(_SHADERS_SRC "${_SHADERS_ROOT}/Shaders")
else()
    set(_SHADERS_SRC "${CMAKE_SOURCE_DIR}/Shaders")
endif()
add_custom_command(TARGET gxlib_example_06 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_SHADERS_SRC}" "$<TARGET_FILE_DIR:gxlib_example_06>/Shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:gxlib_example_06>/Shaders/custom"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Rainbow.hlsl"
        "$<TARGET_FILE_DIR:gxlib_example_06>/Shaders/custom/Rainbow.hlsl"
    COMMENT "Copying Shaders + custom Rainbow.hlsl")
set_target_properties(gxlib_example_06 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:gxlib_example_06>")
if (COMMAND gxlib_copy_runtime_dlls)
    gxlib_copy_runtime_dlls(gxlib_example_06)
endif()
