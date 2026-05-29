# Vendored, MIT-licensed. https://github.com/ocornut/imgui (docking branch)
add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp
    backends/imgui_impl_win32.cpp
    backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

if(WIN32)
    target_link_libraries(imgui PUBLIC d3d11 dxgi dwmapi)
    target_compile_definitions(imgui PUBLIC
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Vendored third-party: silence its warnings without forcing /W4 on it.
if(MSVC)
    target_compile_options(imgui PRIVATE /W1 /wd4100 /wd4267)
else()
    target_compile_options(imgui PRIVATE -w)
endif()
