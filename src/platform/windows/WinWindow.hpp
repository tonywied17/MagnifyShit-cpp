#pragma once

#include "../../util/Geometry.hpp"

#include <Windows.h>

#include <functional>
#include <string>

namespace magshit::platform {

/// @brief Initial creation parameters for a `WinWindow`.
struct WindowDesc
{
    std::wstring title = L"MagnifyShit";
    int width = 1100;
    int height = 700;
};

/// @brief Owns a Win32 top-level window and routes its WndProc to the app.
class WinWindow
{
public:
    using MsgHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM, bool& handled)>;

    /**
     * @brief Construct an empty window wrapper.
     */
    WinWindow();

    /**
     * @brief Destroy the HWND and unregister the window class if needed.
     */
    ~WinWindow();

    WinWindow(const WinWindow&) = delete;
    WinWindow& operator=(const WinWindow&) = delete;

    /**
     * @brief Register the window class and create the HWND.
     * @param hInst Module instance that owns the window class.
     * @param desc Creation title and initial dimensions.
     * @param handler Application message handler invoked from WndProc.
     * @return true on success; false leaves the instance in a destroyed state.
     */
    bool create(HINSTANCE hInst, const WindowDesc& desc, MsgHandler handler);

    /**
     * @brief Show the window.
     * @param nCmdShow Show command, typically forwarded from `WinMain`.
     */
    void show(int nCmdShow);

    /**
     * @brief Access the native window handle.
     * @return HWND, or null before `create()` succeeds.
     */
    HWND handle() const noexcept { return hwnd_; }

    /**
     * @brief Query the current client-area size.
     * @return Client-area dimensions in physical pixels.
     */
    Size clientSize() const noexcept;

    /**
     * @brief Query the client-area rectangle in screen coordinates.
     * @return Client rectangle mapped to screen coordinates.
     */
    Rect clientScreenRect() const noexcept;

    /**
     * @brief Check whether the window uses borderless popup styling.
     * @return true when borderless mode is active.
     */
    bool borderless() const noexcept { return borderless_; }

    /**
     * @brief Switch between borderless popup and standard overlapped frame.
     * @param on true for borderless popup style; false for normal frame style.
     */
    void setBorderless(bool on);

    /**
     * @brief Check whether the window is topmost.
     * @return true when `HWND_TOPMOST` is active.
     */
    bool alwaysOnTop() const noexcept { return topmost_; }

    /**
     * @brief Toggle `HWND_TOPMOST` / `HWND_NOTOPMOST`.
     * @param on true to keep the window above non-topmost windows.
     */
    void setAlwaysOnTop(bool on);

    /**
     * @brief Check whether click-through support is enabled.
     * @return true when click-through styles are active.
     */
    bool clickThrough() const noexcept { return clickThrough_; }

    /**
     * @brief Enable or disable layered click-through window styles.
     * @param on true to enable click-through support; false to restore normal input.
     */
    void setClickThrough(bool on);

    /**
     * @brief Check whether DWM capture exclusion is enabled.
     * @return true when `WDA_EXCLUDEFROMCAPTURE` has been requested.
     */
    bool excludedFromCapture() const noexcept { return excludedFromCapture_; }

    /**
     * @brief Apply or clear `WDA_EXCLUDEFROMCAPTURE`.
     * @param on true to exclude this window from capture when supported by Windows.
     */
    void setExcludedFromCapture(bool on);

    /**
     * @brief Query the borderless resize zone under the cursor.
     * @return Hit-test code such as `HTLEFT`, `HTBOTTOMRIGHT`, or `HTNOWHERE`.
     */
    int resizeHover() const noexcept { return resizeHover_; }

    /**
     * @brief Replace the native title-bar text.
     * @param title New window title.
     */
    void setTitle(const std::wstring& title);

private:
    /**
     * @brief Static Win32 WndProc trampoline.
     * @param hwnd Window receiving the message.
     * @param msg Win32 message identifier.
     * @param wParam Message WPARAM.
     * @param lParam Message LPARAM.
     * @return Win32 message result.
     */
    static LRESULT CALLBACK staticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Instance WndProc implementation.
     * @param hwnd Window receiving the message.
     * @param msg Win32 message identifier.
     * @param wParam Message WPARAM.
     * @param lParam Message LPARAM.
     * @return Win32 message result.
     */
    LRESULT proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Perform resize hit-testing for the custom borderless frame.
     * @param hwnd Window receiving `WM_NCHITTEST`.
     * @param lParam Packed screen-space cursor point from the message.
     * @return Hit-test code for a resize edge/corner, client area, or nowhere.
     */
    LRESULT hitTestBorderless(HWND hwnd, LPARAM lParam) const;

    HINSTANCE inst_ = nullptr;
    HWND hwnd_ = nullptr;
    std::wstring className_;
    MsgHandler handler_;
    bool borderless_ = false;
    bool topmost_ = true;
    bool clickThrough_ = false;
    bool excludedFromCapture_ = false;
    int  resizeHover_ = 0; // HTNOWHERE
};

} // namespace magshit::platform
