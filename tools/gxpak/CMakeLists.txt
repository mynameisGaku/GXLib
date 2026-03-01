cmake_minimum_required(VERSION 3.24)

add_executable(gxpak
    main.cpp
    ${CMAKE_SOURCE_DIR}/GXLib/ThirdParty/lz4.c
)

target_include_directories(gxpak PRIVATE
    ${CMAKE_SOURCE_DIR}/gxformat
    ${CMAKE_SOURCE_DIR}/GXLib/ThirdParty
)

target_compile_features(gxpak PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(gxpak PRIVATE /utf-8)
endif()

# lz4.c: C file, skip PCH
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/GXLib/ThirdParty/lz4.c"
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        LANGUAGE C
)
