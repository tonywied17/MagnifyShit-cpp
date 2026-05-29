#pragma once

#include "../../util/Geometry.hpp"

#include <Windows.h>

#include <functional>
#include <string>

namespace magshit::platform {

/// Initial creation parameters for a `WinWindow`.
struct WindowDesc
{
    std::wstring title = L"MagnifyShit";
    int width = 1100;
    int height = 700;
};

/// Owns a Win32 top-level window and routes its WndProc through a
/// user-supplied callback. Exposes the toggles used by the magnifier
/// (borderless, always-on-top, click-through, exclude-from-capture).
class WinWindow
{
public:
    using MsgHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool& handled)>;

    WinWindow();
    ~WinWindow();

    WinWindow(const WinWindow&) = delete;
    WinWindow& operator=(const WinWindow&) = delete;

    /// Register the window class and create the HWND. Returns false on
    /// failure; the instance is left in a destroyed state.
    bool create(HINSTANCE hInst, const WindowDesc& desc, MsgHandler handler);

    /// Show the window with the given `nCmdShow` (typically forwarded from
    /// `WinMain`).
    void show(int nCmdShow);

    /// Native window handle (nullptr if not created).
    HWND handle() const noexcept { return hwnd_; }

    /// Current client-area size in pixels.
    Size clientSize() const noexcept;

    /// Client-area rectangle in screen coordinates.
    Rect clientScreenRect() const noexcept;

    /// Whether the window is in borderless / popup style.
    bool borderless() const noexcept { return borderless_; }

    /// Switch between borderless popup and standard overlapped frame.
    void setBorderless(bool on);

    /// Whether the window is currently topmost.
    bool alwaysOnTop() const noexcept { return topmost_; }

    /// Toggle `HWND_TOPMOST` / `HWND_NOTOPMOST`.
    void setAlwaysOnTop(bool on);

    /// Whether per-pixel click-through is enabled.
    bool clickThrough() const noexcept { return clickThrough_; }

    /// Enable layered + magnifier-aware click-through. Actual per-pixel
    /// pass-through is done in the application's input handler; this only
    /// sets up the styles.
    void setClickThrough(bool on);

    /// Whether DWM is asked to exclude this window from screen capture.
    bool excludedFromCapture() const noexcept { return excludedFromCapture_; }

    /// Apply or clear `WDA_EXCLUDEFROMCAPTURE` (Windows 10 2004+).
    void setExcludedFromCapture(bool on);

    /// Replace the title-bar text.
    void setTitle(const std::wstring& title);

private:
    static LRESULT CALLBACK staticProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT proc(HWND, UINT, WPARAM, LPARAM);

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    std::wstring className_;
    MsgHandler handler_;
    bool borderless_ = false;
    bool topmost_ = true;
    bool clickThrough_ = false;
    bool excludedFromCapture_ = false;
};

} // namespace magshit::platform
