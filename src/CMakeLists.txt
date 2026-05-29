add_executable(MagnifyShit WIN32
    main.cpp
    app/Application.cpp
    app/Config.cpp
    app/Hotkeys.cpp
    capture/windows/DxgiDuplication.cpp
    platform/windows/WinPlatform.cpp
    platform/windows/WinWindow.cpp
    render/d3d11/D3D11Renderer.cpp
    tools/Eyedropper.cpp
    tools/Screenshot.cpp
    ui/ImGuiBackend.cpp
    ui/MainOverlay.cpp
    ui/SettingsWindow.cpp
    ui/Theme.cpp
    util/Log.cpp
)

target_link_libraries(MagnifyShit PRIVATE
    imgui
    stb
    nlohmann_json
    d3d11
    dxgi
    dxguid
    d3dcompiler
    dwmapi
    dcomp
    windowsapp
    shcore
)

target_include_directories(MagnifyShit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(MagnifyShit PROPERTIES
    VS_DPI_AWARE "PerMonitor"
)

if(MSVC)
    target_compile_options(MagnifyShit PRIVATE /W4 /WX)
endif()
