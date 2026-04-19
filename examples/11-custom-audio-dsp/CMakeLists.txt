cmake_minimum_required(VERSION 3.24)
project(gxlib_example_11 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/utf-8)
endif()
if (NOT TARGET GXLib::GXLib)
    find_package(GXLib REQUIRED)
endif()
add_executable(gxlib_example_11 WIN32 main.cpp)
target_link_libraries(gxlib_example_11 PRIVATE GXLib::GXLib)
if (DEFINED GXLib_DIR)
    get_filename_component(_SHADERS_ROOT "${GXLib_DIR}/.." ABSOLUTE)
    set(_SHADERS_SRC "${_SHADERS_ROOT}/Shaders")
else()
    set(_SHADERS_SRC "${CMAKE_SOURCE_DIR}/Shaders")
endif()
add_custom_command(TARGET gxlib_example_11 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_SHADERS_SRC}" "$<TARGET_FILE_DIR:gxlib_example_11>/Shaders"
    COMMENT "Copying Shaders")

# Copy example-local Assets/ (test_tone.wav for the IXAPO verification test)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    add_custom_command(TARGET gxlib_example_11 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:gxlib_example_11>/Assets"
        COMMENT "Copying example Assets")
endif()
set_target_properties(gxlib_example_11 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:gxlib_example_11>")
if (COMMAND gxlib_copy_runtime_dlls)
    gxlib_copy_runtime_dlls(gxlib_example_11)
endif()
