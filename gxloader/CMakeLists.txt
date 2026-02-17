cmake_minimum_required(VERSION 3.24)

add_library(gxloader STATIC
    model_loader.cpp
    anim_loader.cpp
    bone_matcher.cpp
    pak_loader.cpp
)

target_include_directories(gxloader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/gxformat
    ${CMAKE_SOURCE_DIR}/GXLib/ThirdParty
)

target_compile_features(gxloader PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(gxloader PRIVATE /utf-8)
endif()
