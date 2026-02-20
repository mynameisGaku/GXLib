# GXModelViewer - ImGui-based 3D model viewer
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui)
set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imguizmo)
set(IMNODES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imnodes)
set(IMPLOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/implot)
set(FILEDIALOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ImGuiFileDialog)

# ImGui core sources
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
)

# Extension sources
set(EXTENSION_SOURCES
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
    ${IMNODES_DIR}/imnodes.cpp
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${FILEDIALOG_DIR}/ImGuiFileDialog.cpp
)

# Application sources
file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Exclude ThirdParty from GLOB (they're added separately)
list(FILTER APP_SOURCES EXCLUDE REGEX "ThirdParty/")

# gxconv exporter sources (for integrated GXMD/GXAN export)
set(GXCONV_EXPORT_SOURCES
    ${CMAKE_SOURCE_DIR}/gxconv/exporters/gxmd_exporter.cpp
    ${CMAKE_SOURCE_DIR}/gxconv/exporters/gxan_exporter.cpp
)
set_source_files_properties(${GXCONV_EXPORT_SOURCES}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

add_executable(GXModelViewer WIN32
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
    ${EXTENSION_SOURCES}
    ${GXCONV_EXPORT_SOURCES}
)

target_include_directories(GXModelViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/GXLib
    ${CMAKE_SOURCE_DIR}/gxconv
    ${CMAKE_SOURCE_DIR}/gxformat
    ${CMAKE_SOURCE_DIR}/gxloader
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
    ${IMNODES_DIR}
    ${IMPLOT_DIR}
    ${FILEDIALOG_DIR}
)

target_link_libraries(GXModelViewer PRIVATE GXLib gxloader)

# Copy Shaders and Assets
add_custom_command(TARGET GXModelViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders
        $<TARGET_FILE_DIR:GXModelViewer>/Shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:GXModelViewer>/Assets
)

# VS debugger working directory
set_target_properties(GXModelViewer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:GXModelViewer>
)

# Disable PCH for ImGui/extension sources (they don't use GXLib's pch)
set_source_files_properties(${IMGUI_SOURCES} ${EXTENSION_SOURCES}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)
