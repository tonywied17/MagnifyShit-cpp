/*
 * File: c:\Users\tonyw\source\repos\MagnifyShit\MagnifyShit\MagnifyShit.cpp
 * Project: c:\Users\tonyw\source\repos\MaginfyShit\MagnifyShit
 * Created Date: Saturday April 26th 2025
 * Author: Tony Wiedman
 * -----
 * Last Modified: Sun April 27th 2025 5:49:48 
 * Modified By: Tony Wiedman
 * -----
 * Copyright (c) 2025 MolexWorks
 */

#include <windows.h>
#include "resource.h"
#include <magnification.h>
#pragma comment(lib, "Magnification.lib")

// Constants
constexpr int FOOTER_HEIGHT = 33;
constexpr int REFRESH_RATE_MS = 8;

// Global state
namespace
{
    HINSTANCE g_hInst;
    HWND g_hMainWnd, g_hMag;
    int g_zoom = 2;
    bool g_followMouse = false;
    bool g_attachWindowToMouse = false;
    bool g_showFooter = true;
}

// Forward declarations
void UpdateMagnifier();
void ToggleBorderlessMode(HWND hWnd);
void DrawFooter(HDC hdc);
void CenterWindowOnMouse(HWND hWnd);

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
    if (g_followMouse)
    {
        GetCursorPos(&sourcePos);
        sourcePos.x -= sourceWidth / 2;
        sourcePos.y -= sourceHeight / 2;
    }
    else
    {
        sourcePos.x = ptClientTopLeft.x + (width - sourceWidth) / 2;
        sourcePos.y = ptClientTopLeft.y + (height - sourceHeight) / 2;
    }

    const int vsx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vsy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vsw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vsh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    sourcePos.x = max(vsx, min(sourcePos.x, vsx + vsw - sourceWidth));
    sourcePos.y = max(vsy, min(sourcePos.y, vsy + vsh - sourceHeight));

    MagSetWindowSource(g_hMag, {sourcePos.x, sourcePos.y, sourceWidth, sourceHeight});

    MAGTRANSFORM transform{};
    transform.v[0][0] = transform.v[1][1] = static_cast<float>(g_zoom);
    MagSetWindowTransform(g_hMag, &transform);
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

    const wchar_t *texts[] = {
        L" [Ctrl+B]: Borderless Mode",
        L" [Ctrl+0]: Reset Zoom  [Ctrl +/-]: Zoom In/Out ",
        L" [Ctrl+W]: Cursor/Window Magnifier ",
        L" [Borderless Mode]: Click to attach/detach the window to cursor "};

    const UINT formats[] = {
        DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_TOP,
        DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM,
        DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_TOP,
        DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM};

    SetTextColor(hdc, RGB(0, 0, 255));
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < sizeof(texts) / sizeof(texts[0]); ++i)
    {
        DrawTextW(hdc, texts[i], -1, &rc, formats[i]);
    }

    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, OPAQUE);
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
 * @brief Toggles the borderless mode of the main window.
 *
 * @param hWnd Handle to the main window.
 */
void ToggleBorderlessMode(HWND hWnd)
{
    const LONG style = GetWindowLong(hWnd, GWL_STYLE);
    const bool isBorderless = (style & WS_OVERLAPPEDWINDOW) == 0;

    g_showFooter = isBorderless;

    DWORD exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    SetWindowLong(hWnd, GWL_STYLE,
                  isBorderless ? (style | WS_OVERLAPPEDWINDOW) & ~WS_POPUP
                               : (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);

    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED | WS_EX_LAYERED);

    g_attachWindowToMouse = false;

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
        if (g_attachWindowToMouse)
        {
            POINT pt;
            GetCursorPos(&pt);
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
        UpdateMagnifier();
        break;

    case WM_LBUTTONDOWN:
        if (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP)
        {
            g_attachWindowToMouse = !g_attachWindowToMouse;
            if (g_attachWindowToMouse)
            {
                CenterWindowOnMouse(hWnd);
            }
            else
            {
                g_followMouse = false;
            }
        }
        UpdateMagnifier();
        break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_zoom = max(1, g_zoom + (delta > 0 ? 1 : -1));
        UpdateMagnifier();
        break;
    }

    case WM_KEYDOWN:
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
                g_followMouse = !g_followMouse;
                UpdateMagnifier();
                break;
            case VK_ADD:
            case VK_OEM_PLUS:
                g_zoom++;
                UpdateMagnifier();
                break;
            case VK_SUBTRACT:
            case VK_OEM_MINUS:
                g_zoom = max(1, g_zoom - 1);
                UpdateMagnifier();
                break;
            case '0':
            case VK_NUMPAD0:
                g_zoom = 1;
                UpdateMagnifier();
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