#pragma once

#include "Theme.hpp"

#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace magshit::ui {

/// @brief Owns the ImGui context and Win32/DX11 backend bindings.
class ImGuiBackend
{
public:
    /**
     * @brief Construct an uninitialized ImGui backend.
     */
    ImGuiBackend() = default;

    /**
     * @brief Shut down ImGui if it is still initialized.
     */
    ~ImGuiBackend();

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;

    /**
     * @brief Initialize ImGui for a Win32 window and D3D11 device/context.
     * @param hwnd Window handle used by the Win32 backend.
     * @param device D3D11 device used by the DX11 backend.
     * @param context Immediate D3D11 context used by the DX11 backend.
     * @return true on success; false tears down any partial initialization.
     */
    bool init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);

    /**
     * @brief Tear down ImGui and both platform/render backends.
     */
    void shutdown();

    /**
     * @brief Start a new ImGui frame.
     */
    void beginFrame();

    /**
     * @brief Render queued ImGui draw data through the DX11 backend.
     */
    void endFrame();

    /**
     * @brief Forward a Win32 message to ImGui.
     * @param hwnd Window receiving the message.
     * @param msg Win32 message identifier.
     * @param wParam Message WPARAM.
     * @param lParam Message LPARAM.
     * @return true when ImGui consumed the message.
     */
    bool handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Rebuild the font atlas for a DPI scale.
     * @param dpiScale Scale relative to 96 DPI.
     */
    void rebuildFonts(float dpiScale);

    /**
     * @brief Switch between light, dark, and auto styling.
     * @param mode Requested theme mode.
     */
    void setThemeMode(ThemeMode mode);

    /**
     * @brief Query the requested theme mode.
     * @return User-selected theme mode; may be `Auto`.
     */
    ThemeMode themeMode() const noexcept { return themeMode_; }

    /**
     * @brief Check whether ImGui is initialized.
     * @return true once `init()` has succeeded and before `shutdown()`.
     */
    bool ready() const noexcept { return ready_; }

private:
    bool ready_ = false;
    float dpiScale_ = 1.0f;
    ThemeMode themeMode_ = ThemeMode::Light;
};

} // namespace magshit::ui
