#include "ImGuiBackend.hpp"

#include "Theme.hpp"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <cmath>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace magshit::ui {

ImGuiBackend::~ImGuiBackend()
{
    shutdown();
}

bool ImGuiBackend::init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplWin32_Init(hwnd))
    {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplDX11_Init(device, context))
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    ready_ = true;
    return true;
}

void ImGuiBackend::shutdown()
{
    if (!ready_)
    {
        return;
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ready_ = false;
}

void ImGuiBackend::beginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::endFrame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiBackend::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!ready_)
    {
        return false;
    }
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
}

void ImGuiBackend::rebuildFonts(float dpiScale)
{
    if (!ready_ || dpiScale <= 0.0f)
    {
        return;
    }
    dpiScale_ = dpiScale;

    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig cfg{};
    cfg.SizePixels = std::floor(16.0f * dpiScale);
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;
    io.Fonts->AddFontDefault(&cfg);
    io.Fonts->Build();

    ImGui_ImplDX11_CreateDeviceObjects();

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle{};
    Theme::apply(themeMode_);
    style.ScaleAllSizes(dpiScale);
    io.FontGlobalScale = 1.0f;
}

void ImGuiBackend::setThemeMode(ThemeMode mode)
{
    themeMode_ = mode;
    Theme::apply(mode);
}

} // namespace magshit::ui
