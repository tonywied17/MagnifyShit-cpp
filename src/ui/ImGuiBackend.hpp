#pragma once

#include "Theme.hpp"

#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace magshit::ui {

/// Owns the ImGui context plus the Win32 + DX11 platform/render backends
/// and handles font rebuilds on DPI changes.
class ImGuiBackend
{
public:
    ImGuiBackend() = default;
    ~ImGuiBackend();

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;

    /// Initialise ImGui against the given window/device. Returns false on
    /// failure (already-initialised state will be torn down).
    bool init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);

    /// Tear down ImGui and both backends. Idempotent.
    void shutdown();

    /// Start a new ImGui frame.
    void beginFrame();

    /// Render the queued draw data and present it through the DX11 backend.
    void endFrame();

    /// Forward a Win32 message to `ImGui_ImplWin32_WndProcHandler`. Returns
    /// true if the message was consumed by ImGui.
    bool handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// Rebuild the font atlas at the given DPI scale (1.0 = 96 dpi).
    void rebuildFonts(float dpiScale);

    /// Switch between light/dark/auto styling.
    void setThemeMode(ThemeMode mode);

    /// Current theme mode (the user's choice, may be `Auto`).
    ThemeMode themeMode() const noexcept { return themeMode_; }

    /// True once `init` has succeeded.
    bool ready() const noexcept { return ready_; }

private:
    bool ready_ = false;
    float dpiScale_ = 1.0f;
    ThemeMode themeMode_ = ThemeMode::Light;
};

} // namespace magshit::ui
