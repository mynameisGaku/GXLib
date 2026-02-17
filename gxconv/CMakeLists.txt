cmake_minimum_required(VERSION 3.24)

set(GXCONV_SOURCES
    main.cpp
    converter.cpp
    importers/obj_importer.cpp
    importers/fbx_importer.cpp
    importers/gltf_importer.cpp
    exporters/gxmd_exporter.cpp
    exporters/gxan_exporter.cpp
    ${CMAKE_SOURCE_DIR}/GXLib/ThirdParty/ufbx.c
)

add_executable(gxconv ${GXCONV_SOURCES})

target_include_directories(gxconv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/gxformat
    ${CMAKE_SOURCE_DIR}/GXLib/ThirdParty
)

target_compile_features(gxconv PRIVATE cxx_std_20)

target_compile_definitions(gxconv PRIVATE
    GXCONV_HAS_FBX
    GXCONV_HAS_GLTF
)

if(MSVC)
    target_compile_options(gxconv PRIVATE /utf-8)
endif()

# ufbx.c: C file, skip PCH
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/GXLib/ThirdParty/ufbx.c"
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        LANGUAGE C
)
