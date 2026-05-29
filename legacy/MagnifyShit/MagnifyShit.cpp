/*
 * File: c:\Users\tonyw\source\repos\MagnifyShit\MagnifyShit\MagnifyShit.cpp
 * Project: c:\Users\tonyw\source\repos\MaginfyShit\MagnifyShit
 * Created Date: Saturday April 26th 2025
 * Author: Tony Wiedman
 * -----
 * Last Modified: Mon April 28th 2025 10:22:03 
 * Modified By: Tony Wiedman
 * -----
 * Copyright (c) 2025 MolexWorks
 */

// @ Includes  ---------------------------------------------------------------------------------------

#include "MagnifyShit.h"

// @ Constants  --------------------------------------------------------------------------------------

constexpr int FOOTER_HEIGHT = 33;
constexpr int REFRESH_RATE_MS = 1;

// @ Enumerations  -----------------------------------------------------------------------------------

enum class MagnifierMode
{
    FollowMouse,   ///< The magnifier follows the mouse cursor
    AttachToMouse, ///< The magnifier is attached to the mouse cursor
    None           ///< No specific magnifier mode
};

enum class BorderlessMode
{
    Enabled, ///< Borderless mode enabled
    Disabled ///< Borderless mode disabled
};

enum class ZoomLevel
{
    Default, ///< Default zoom level 2.0x
    ZoomIn,  ///< Zoom in by a factor of 0.1x
    ZoomOut, ///< Zoom out by a factor of 0.1x
    Reset    ///< Reset zoom to 1.0x zoom level
};

// @ Global Variables  -------------------------------------------------------------------------------

namespace
{
    HINSTANCE g_hInst;
    HWND g_hMainWnd, g_hMag;
    float g_zoom = 2.0f;
    MagnifierMode g_magnifierMode = MagnifierMode::None;
    BorderlessMode g_borderlessMode = BorderlessMode::Disabled;
    ZoomLevel g_zoomLevel = ZoomLevel::Default;
    bool g_showFooter = true;
}

// @ Function Prototypes  ----------------------------------------------------------------------------

void UpdateMagnifier();
void CenterWindowOnMouse(HWND hWnd);
void HandleZoom(WPARAM wParam);
void ToggleBorderlessMode(HWND hWnd);
void DrawFooter(HDC hdc);
void UpdateWindowTitle();
void DrawTextWithFormat(HDC hdc, const wchar_t *text, RECT &rc, UINT format);

// @ Window Procedure  -------------------------------------------------------------------------------

/**
 * @brief Handle messages sent to the main window.
 *
 * @param hWnd Handle to the window receiving the message.
 * @param msg Message identifier.
 * @param wParam Additional message information.
 * @param lParam Additional message information.
 * @return The result of the message processing.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static RECT lastPosition{};

    switch (msg)
    {
    case WM_CREATE:
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_COMPOSITED);
        SetTimer(hWnd, 1, REFRESH_RATE_MS, nullptr);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_SIZE:
    case WM_MOVE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        const int magHeight = rc.bottom - (g_showFooter ? FOOTER_HEIGHT : 0);
        SetWindowPos(g_hMag, nullptr, 0, 0, rc.right, magHeight, SWP_NOZORDER);
        UpdateMagnifier();
        break;
    }

    case WM_TIMER:
    {
        g_magnifierMode == MagnifierMode::AttachToMouse
            ? [&]()
            {
                POINT pt;
                GetCursorPos(&pt);
                RECT rc;
                GetWindowRect(hWnd, &rc);

                int centerX = (rc.left + rc.right) / 2;
                int centerY = (rc.top + rc.bottom) / 2;

                (pt.x != centerX || pt.y != centerY)
                    ? static_cast<void>(SetWindowPos(
                        hWnd,
                        nullptr,
                        pt.x - (rc.right - rc.left) / 2,
                        pt.y - (rc.bottom - rc.top) / 2,
                        0, 0,
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS
                    ))
                    : static_cast<void>(0);
            }()
            : static_cast<void>(0);

        UpdateMagnifier();
        break;
    }

    case WM_LBUTTONDOWN:
    {
        g_magnifierMode = GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP
            ? (g_magnifierMode == MagnifierMode::AttachToMouse ? MagnifierMode::None : MagnifierMode::AttachToMouse)
            : g_magnifierMode;

        g_magnifierMode == MagnifierMode::AttachToMouse ? CenterWindowOnMouse(hWnd) : (void)0;

        UpdateMagnifier();
        break;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        g_zoomLevel = ZoomLevel::Default;
        HandleZoom(wParam);
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            const LONG style = GetWindowLong(hWnd, GWL_STYLE);
            const bool isBorderless = (style & WS_OVERLAPPEDWINDOW) == 0;
            isBorderless ? ToggleBorderlessMode(hWnd) : void();
            break;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            switch (LOWORD(wParam))
            {
            case 'B':
            case 'b':
                ToggleBorderlessMode(hWnd);
                break;
            case 'W':
            case 'w':
                g_magnifierMode = (g_magnifierMode == MagnifierMode::FollowMouse)
                                      ? MagnifierMode::None
                                      : MagnifierMode::FollowMouse;
                UpdateMagnifier();
                break;
            case VK_ADD:
            case VK_OEM_PLUS:
                g_zoomLevel = ZoomLevel::ZoomIn;
                HandleZoom(wParam);
                break;
            case VK_SUBTRACT:
            case VK_OEM_MINUS:
                g_zoomLevel = ZoomLevel::ZoomOut;
                HandleZoom(wParam);
                break;
            case '0':
            case VK_NUMPAD0:
                g_zoomLevel = ZoomLevel::Reset;
                HandleZoom(wParam);
                break;
            }
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UpdateMagnifier();
        DrawFooter(hdc);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// @ Magnifier Functions  ----------------------------------------------------------------------------

/**
 * @brief Handles zoom level changes based on mouse wheel input.
 *
 * @param wParam The wParam parameter from the mouse wheel or keyboard event.
 */
static void HandleZoom(WPARAM wParam)
{
    int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

    g_zoom = (g_zoomLevel == ZoomLevel::ZoomIn)
                 ? g_zoom + 0.1f
                 : (g_zoomLevel == ZoomLevel::ZoomOut)
                       ? max(1.0f, g_zoom - 0.1f)
                       : (g_zoomLevel == ZoomLevel::Reset)
                             ? 1.0f
                             : (g_zoomLevel == ZoomLevel::Default)
                                   ? (wheelDelta > 0 ? g_zoom + 0.1f : max(1.0f, g_zoom - 0.1f))
                                   : g_zoom;

    g_zoom = min(5.0f, max(1.0f, g_zoom));

    UpdateWindowTitle();
    UpdateMagnifier();
}

/**
 * @brief Updates the magnifier's source region and transformation matrix.
 */
void UpdateMagnifier()
{
    RECT rcClient;
    GetClientRect(g_hMainWnd, &rcClient);

    POINT ptClientTopLeft{};
    ClientToScreen(g_hMainWnd, &ptClientTopLeft);

    const int width = rcClient.right;
    const int height = rcClient.bottom - (g_showFooter ? FOOTER_HEIGHT : 0);
    const int sourceWidth = width / g_zoom;
    const int sourceHeight = height / g_zoom;

    POINT sourcePos{};
    switch (g_magnifierMode)
    {
    case MagnifierMode::FollowMouse:
        GetCursorPos(&sourcePos);
        sourcePos.x -= sourceWidth / 2;
        sourcePos.y -= sourceHeight / 2;
        break;
    case MagnifierMode::AttachToMouse:
        GetCursorPos(&sourcePos);
        sourcePos.x -= sourceWidth / 2;
        sourcePos.y -= sourceHeight / 2;
        break;
    case MagnifierMode::None:
        sourcePos.x = ptClientTopLeft.x + (width - sourceWidth) / 2;
        sourcePos.y = ptClientTopLeft.y + (height - sourceHeight) / 2;
        break;
    }

    const int vsx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vsy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vsw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vsh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    sourcePos.x = max(vsx, min(sourcePos.x, vsx + vsw - sourceWidth));
    sourcePos.y = max(vsy, min(sourcePos.y, vsy + vsh - sourceHeight));

    MagSetWindowSource(g_hMag, {sourcePos.x, sourcePos.y, sourceWidth, sourceHeight});

    MAGTRANSFORM transform{};
    transform.v[0][0] = transform.v[1][1] = g_zoom;
    MagSetWindowTransform(g_hMag, &transform);
}

// @ Window Helper Functions  ------------------------------------------------------------------------

/**
 * @brief Toggles the borderless mode of the main window.
 *
 * @param hWnd Handle to the main window.
 */
void ToggleBorderlessMode(HWND hWnd)
{
    const LONG style = GetWindowLong(hWnd, GWL_STYLE);
    const bool isBorderless = (style & WS_OVERLAPPEDWINDOW) == 0;

    MagnifierMode previousMode = g_magnifierMode;

    BorderlessMode currentMode = isBorderless ? BorderlessMode::Enabled : BorderlessMode::Disabled;

    g_showFooter = (currentMode == BorderlessMode::Enabled);

    DWORD exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    SetWindowLong(hWnd, GWL_STYLE,
                  currentMode == BorderlessMode::Enabled
                      ? (style | WS_OVERLAPPEDWINDOW) & ~WS_POPUP
                      : (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);

    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED | WS_EX_LAYERED);

    g_magnifierMode = (currentMode == BorderlessMode::Enabled)
                          ? MagnifierMode::None
                          : (previousMode == MagnifierMode::FollowMouse ? MagnifierMode::FollowMouse : MagnifierMode::None);

    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS);
    SetFocus(hWnd);

    RECT rc;
    GetClientRect(hWnd, &rc);
    const int magHeight = rc.bottom - (g_showFooter ? FOOTER_HEIGHT : 0);
    SetWindowPos(g_hMag, nullptr, 0, 0, rc.right, magHeight,
                 SWP_NOZORDER | SWP_ASYNCWINDOWPOS);

    InvalidateRect(hWnd, nullptr, TRUE);
}

/**
 * @brief Updates the window title with the current zoom level.
 */
void UpdateWindowTitle()
{
    wchar_t title[256];
    swprintf(title, 256, L"Magnify Shit - Zoom: %.1fx", g_zoom);
    SetWindowTextW(g_hMainWnd, title);
}

/**
 * @brief Updates the position of the window to align with the given cursor position.
 *
 * @param hWnd Handle to the window to be repositioned.
 * @param pt The cursor position to center the window on.
 */
static void UpdateWindowPosition(HWND hWnd, const POINT &pt)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);

    if (pt.x != (rc.left + rc.right) / 2 || pt.y != (rc.top + rc.bottom) / 2)
    {
        SetWindowPos(hWnd, nullptr,
                     pt.x - (rc.right - rc.left) / 2,
                     pt.y - (rc.bottom - rc.top) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
    }
}

/**
 * @brief Centers the window on the mouse cursor
 *
 * @param hWnd Handle to the window to be centered.
 */
void CenterWindowOnMouse(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    RECT rc;
    GetWindowRect(hWnd, &rc);
    const int winWidth = rc.right - rc.left;
    const int winHeight = rc.bottom - rc.top;

    SetWindowPos(hWnd, nullptr,
                 pt.x - winWidth / 2, pt.y - winHeight / 2,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/**
 * @brief Draws the instruction footer on the main window.
 *
 * @param hdc Handle to the device context for drawing.
 */
void DrawFooter(HDC hdc)
{
    if (!g_showFooter)
        return;

    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    rc.top = rc.bottom - FOOTER_HEIGHT;

    const struct FooterText
    {
        const wchar_t *text;
        UINT format;
    } footerTexts[] = {
        {L" [Ctrl+B]: Borderless Mode", DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_TOP},
        {L" [Ctrl+0]: Reset Zoom  [Ctrl +/-]: Zoom In/Out ", DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM},
        {L" [Ctrl+W]: Cursor/Window Magnifier ", DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_TOP},
        {L" [Borderless Mode]: Click to attach/detach the window to cursor ", DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM}};

    for (const auto &footer : footerTexts)
    {
        DrawTextWithFormat(hdc, footer.text, rc, footer.format);
    }
}

/**
 * @brief Draws text with the specified format.
 *
 * @param hdc Handle to the device context for drawing.
 * @param text The text to be drawn.
 * @param rc The rectangle in which to draw the text.
 * @param format The format for drawing the text.
 */
void DrawTextWithFormat(HDC hdc, const wchar_t *text, RECT &rc, UINT format)
{
    SetTextColor(hdc, RGB(0, 0, 255));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text, -1, &rc, format);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, OPAQUE);
}

// @ Entry Point  ------------------------------------------------------------------------------------

/**
 * @brief Entry point for the application.
 *
 * @param hInst Handle to the instance of the application.
 * @param nCmdShow Show command for the window.
 * @return Exit code of the application.
 */
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    g_hInst = hInst;

    if (!MagInitialize())
    {
        return -1;
    }

    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0, 0,
        hInst,
        nullptr,
        LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
        nullptr,
        L"MagHost",
        nullptr};
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_COMPOSITED,
        wc.lpszClassName,
        L"Magnify Shit",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 925, 333,
        nullptr, nullptr, hInst, nullptr);

    HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAGNIFYSHIT));
    SetClassLongPtr(g_hMainWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hIcon));
    SendMessage(g_hMainWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));

    SetLayeredWindowAttributes(g_hMainWnd, 0, 255, LWA_ALPHA);
    ShowWindow(g_hMainWnd, nCmdShow);

    g_hMag = CreateWindowW(
        WC_MAGNIFIER,
        nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        g_hMainWnd,
        nullptr,
        hInst,
        nullptr);

    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    const int magHeight = rc.bottom - (g_showFooter ? FOOTER_HEIGHT : 0);
    SetWindowPos(g_hMag, nullptr, 0, 0, rc.right, magHeight, SWP_NOZORDER);

    UpdateMagnifier();
    InvalidateRect(g_hMainWnd, nullptr, TRUE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    MagUninitialize();
    return static_cast<int>(msg.wParam);
}
