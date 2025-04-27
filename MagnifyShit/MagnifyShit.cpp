/*
 * File: c:\Users\tonyw\source\repos\MagnifyShit\MagnifyShit\MagnifyShit.cpp
 * Project: c:\Users\tonyw\source\repos\MagnifyShit\MagnifyShit
 * Created Date: Saturday April 26th 2025
 * Author: Tony Wiedman
 * -----
 * Last Modified: Sat April 26th 2025 9:35:28 
 * Modified By: Tony Wiedman
 * -----
 * Copyright (c) 2025 MolexWorks
 */

#include <windows.h>
#include "resource.h"
#include <magnification.h>
#pragma comment(lib, "Magnification.lib")

HINSTANCE   g_hInst;                /**< Application instance handle */
HWND        g_hMainWnd, g_hMag;     /**< Handles for main and magnifier windows */
int         g_zoom = 2;             /**< Magnification zoom level */
bool        g_followMouse = false;  /**< Flag to toggle mouse-follow mode */
bool g_attachWindowToMouse = false;  /**< Flag to toggle attaching the window to the mouse */
bool g_showFooter = true;
const int   FOOTER_H = 33;          /**< Height of the instruction footer */
void ToggleBorderlessMode(HWND hWnd);   /**< Function to toggle borderless mode */

/**
 * @brief Updates the magnifier's source region and transformation matrix.
 */
void UpdateMag()
{
    RECT rcWin;
    GetWindowRect(g_hMainWnd, &rcWin);
    int w = rcWin.right - rcWin.left;
    int h = rcWin.bottom - rcWin.top - (g_showFooter ? FOOTER_H : 0);
    int sw = w / g_zoom, sh = h / g_zoom;
    int sx, sy;

    if (g_followMouse || g_attachWindowToMouse)
    {
        POINT pt;
        GetCursorPos(&pt);
        sx = pt.x - sw / 2;
        sy = pt.y - sh / 2;

        if (g_attachWindowToMouse && (GetWindowLong(g_hMainWnd, GWL_STYLE) & WS_POPUP)) {
            SetWindowPos(g_hMainWnd, NULL, sx, sy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    else
    {
        sx = rcWin.left + (w - sw) / 2;
        sy = rcWin.top + (h - sh) / 2;
    }

    int vsx = GetSystemMetrics(SM_XVIRTUALSCREEN),
        vsy = GetSystemMetrics(SM_YVIRTUALSCREEN),
        vsw = GetSystemMetrics(SM_CXVIRTUALSCREEN),
        vsh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    sx = max(vsx, min(sx, vsx + vsw - sw));
    sy = max(vsy, min(sy, vsy + vsh - sh));

    MagSetWindowSource(g_hMag, { sx, sy, sw, sh });
    MAGTRANSFORM x = { 0 };
    x.v[0][0] = x.v[1][1] = (float)g_zoom;
    MagSetWindowTransform(g_hMag, &x);
}

/**
 * @brief Draws the instruction footer on the main window.
 *
 * @param hdc Handle to the device context for drawing.
 */
static void DrawFooter(HDC hdc)
{
    if (!g_showFooter) return;

    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    rc.top = rc.bottom - FOOTER_H;
    const wchar_t* text1 = L" [Ctrl+B]: Borderless Mode";
    const wchar_t* text2 = L" [Ctrl+0]: Reset Zoom  [Ctrl +/-]: Zoom In/Out ";
    const wchar_t* text3 = L" [Ctrl+W]: Cursor/Window Magnifier ";
    const wchar_t* text4 = L" [Borderless Mode]: Click to attach/detach the window to cursor ";
   

    SetTextColor(hdc, RGB(0, 0, 255));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text1, -1, &rc, DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_TOP);
    DrawTextW(hdc, text2, -1, &rc, DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM);
	DrawTextW(hdc, text3, -1, &rc, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_TOP);
	DrawTextW(hdc, text4, -1, &rc, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM);
	
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, OPAQUE);

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
    static RECT lastPosition = { 0, 0, 0, 0 };

    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hWnd, 1, 8, NULL);  // ~125 Hz refresh
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_SIZE: case WM_MOVE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int magHeight = rc.bottom;
        if (g_showFooter)
            magHeight -= FOOTER_H;

        SetWindowPos(g_hMag, NULL, 0, 0, rc.right, magHeight, SWP_NOZORDER);
    }
    UpdateMag();
    break;

    case WM_TIMER:
        UpdateMag();
        break;

    case WM_LBUTTONDOWN:                                                    /** Left Mouse click for borderless mode */
        if (GetWindowLong(g_hMainWnd, GWL_STYLE) & WS_POPUP) {
            if (g_attachWindowToMouse) {
                g_attachWindowToMouse = false;
            }
            else {
                g_attachWindowToMouse = true;

                POINT pt;
                GetCursorPos(&pt);

                lastPosition.left = pt.x - (lastPosition.right - lastPosition.left) / 2;
                lastPosition.top = pt.y - (lastPosition.bottom - lastPosition.top) / 2;

                SetWindowPos(g_hMainWnd, NULL, lastPosition.left, lastPosition.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
        }
        UpdateMag();
        break;

    case WM_MOUSEWHEEL:                                                     /** ScrollWheel (Zoom In/Zoom Out) */
    {
        int d = GET_WHEEL_DELTA_WPARAM(wParam);
        if (d > 0) g_zoom++; else if (g_zoom > 1) g_zoom--;
        UpdateMag();
    }
    break;

    case WM_MOUSEHWHEEL:
    {
        int d = GET_WHEEL_DELTA_WPARAM(wParam);
        if (d > 0) g_zoom++; else if (g_zoom > 1) g_zoom--;
        UpdateMag();
    }
    break;

	//@ Handle Ctrl B, Ctrl W, Ctrl +, Ctrl -, Ctrl 0
    case WM_KEYDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            if (wParam == 'B' || wParam == 'b') {                           /**< Ctrl + B (toggle borderless mode) */
                g_attachWindowToMouse = false;
                g_followMouse = false;
                ToggleBorderlessMode(g_hMainWnd);
                UpdateMag();
            }
            else if (wParam == 'W' || wParam == 'w') {                      /**< Ctrl + W (toggle follow mode) */
                g_followMouse = !g_followMouse;
                UpdateMag();
            }
            else if (wParam == VK_ADD || wParam == VK_OEM_PLUS) {           /**< Ctrl + + (Zoom in) */
                g_zoom++;
                UpdateMag();
            }
            else if (wParam == VK_SUBTRACT || wParam == VK_OEM_MINUS) {     /**< Ctrl + - (Zoom out) */
                if (g_zoom > 1)
                    g_zoom--;
                UpdateMag();
            }
            else if (wParam == '0' || wParam == VK_NUMPAD0) {               /**< Ctrl + 0 or Ctrl + NumPad 0 (Reset zoom to 1) */
                g_zoom = 1;
                UpdateMag();
            }
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UpdateMag();
        DrawFooter(hdc);
        EndPaint(hWnd, &ps);
    }
    break;

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
 * @brief Toggles the borderless mode of the main window.
 *
 * @param hWnd Handle to the main window.
 */
void ToggleBorderlessMode(HWND hWnd) {
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW | WS_POPUP);
        g_showFooter = false;
    }
    else {
        SetWindowLong(hWnd, GWL_STYLE, (style & ~WS_POPUP) | WS_OVERLAPPEDWINDOW);
        g_showFooter = true;
    }

    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    SetFocus(hWnd);

    RECT rc;
    GetClientRect(hWnd, &rc);
    int magHeight = rc.bottom;
    if (g_showFooter)
        magHeight -= FOOTER_H;
    SetWindowPos(g_hMag, NULL, 0, 0, rc.right, magHeight, SWP_NOZORDER);

    InvalidateRect(hWnd, NULL, TRUE);
}

/**
 * @brief Entry point for the application.
 *
 * @param hInst Handle to the application instance.
 * @param hPrevInstance Not used.
 * @param lpCmdLine Command line arguments.
 * @param nCmdShow Show window command.
 * @return Exit code of the application.
 */
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    g_hInst = hInst;
    MagInitialize();

    WNDCLASSEXW wc{ sizeof(wc), CS_HREDRAW | CS_VREDRAW, WndProc,
                    0, 0, hInst, NULL, LoadCursor(NULL, IDC_ARROW),
                    (HBRUSH)(COLOR_BTNFACE + 1), NULL, L"MagHost", NULL };
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(
        WS_EX_LAYERED,
        wc.lpszClassName, L"Magnify Shit",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 925, 333,
        NULL, NULL, hInst, NULL);

    HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAGNIFYSHIT));


    SetClassLongPtr(g_hMainWnd, GCLP_HICON, (LONG_PTR)hIcon);
    SendMessage(g_hMainWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    SetLayeredWindowAttributes(g_hMainWnd, 0, 255, LWA_ALPHA);
    ShowWindow(g_hMainWnd, nCmdShow);

    g_hMag = CreateWindowW(
        WC_MAGNIFIER, NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        g_hMainWnd, NULL, hInst, NULL);

    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    int magHeight = rc.bottom;
    if (g_showFooter)
        magHeight -= FOOTER_H;

    SetWindowPos(g_hMag, NULL, 0, 0, rc.right, magHeight, SWP_NOZORDER);

    MagInitialize();

    UpdateMag();
    InvalidateRect(g_hMainWnd, NULL, TRUE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    MagUninitialize();
    return (int)msg.wParam;
}
