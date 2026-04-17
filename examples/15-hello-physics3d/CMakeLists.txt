cmake_minimum_required(VERSION 3.20)
project(gxlib_example_15 LANGUAGES CXX)

if(NOT TARGET GXLib::GXLib)
    find_package(GXLib REQUIRED)
endif()

add_executable(gxlib_example_15 WIN32 main.cpp)
target_link_libraries(gxlib_example_15 PRIVATE GXLib::GXLib)
set_target_properties(gxlib_example_15 PROPERTIES
    FOLDER "Examples"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")

if(DEFINED GXLib_DIR)
    set(SHADER_SRC "${GXLib_DIR}/../Shaders")
else()
    set(SHADER_SRC "${CMAKE_SOURCE_DIR}/Shaders")
endif()
add_custom_command(TARGET gxlib_example_15 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_SRC}" "$<TARGET_FILE_DIR:gxlib_example_15>/Shaders"
    COMMENT "Copying Shaders")
if(COMMAND gxlib_copy_runtime_dlls)
    gxlib_copy_runtime_dlls(gxlib_example_15)
endif()
