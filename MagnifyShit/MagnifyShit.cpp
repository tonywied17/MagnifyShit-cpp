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
const int   FOOTER_H = 28;          /**< Height of the instruction footer */

/**
 * @brief Updates the magnifier's source region and transformation matrix.
 */
void UpdateMag()
{
    RECT rcWin;
    GetWindowRect(g_hMainWnd, &rcWin);
    int w = rcWin.right - rcWin.left, h = rcWin.bottom - rcWin.top - FOOTER_H;
    int sw = w / g_zoom, sh = h / g_zoom;
    int sx, sy;

    if (g_followMouse)
    {
        POINT pt;
        GetCursorPos(&pt);
        sx = pt.x - sw / 2;
        sy = pt.y - sh / 2;
    }
    else
    {
        sx = rcWin.left + (w - sw) / 2;
        sy = rcWin.top + (h - sh) / 2;
    }

    // Clamp to virtual-screen bounds
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
    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    rc.top = rc.bottom - FOOTER_H;
    const wchar_t* text1 = L"(Scroll Up/Down) or (Ctrl+/-) to zoom ";
    const wchar_t* text2 = L"(Click Window) or (Ctrl+W) to toggle cursor/window follow ";
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text1, -1, &rc, DT_RIGHT | DT_NOCLIP | DT_SINGLELINE);
	DrawTextW(hdc, text2, -1, &rc, DT_RIGHT | DT_NOCLIP | DT_SINGLELINE | DT_BOTTOM);
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
    switch (msg)
    {
    case WM_CREATE:
        //SetTimer(hWnd, 1, 16, NULL);               /**< Set timer for ~60 Hz refresh */
        SetTimer(hWnd, 1, 8, NULL);                  /**< ~125 Hz refresh */
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_SIZE: case WM_MOVE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        SetWindowPos(g_hMag, NULL, 0, 0, rc.right, rc.bottom - FOOTER_H, SWP_NOZORDER);
    }
    UpdateMag();
    //InvalidateRect(hWnd, NULL, TRUE);
    break;

    case WM_TIMER:
        UpdateMag();
        break;

    case WM_LBUTTONDOWN:                        /**< Click window toggles follow */
        g_followMouse = !g_followMouse;
        UpdateMag();
        break;

    case WM_MOUSEWHEEL:
    {
        int d = GET_WHEEL_DELTA_WPARAM(wParam);  /**< Vertical wheel */
        if (d > 0) g_zoom++; else if (g_zoom > 1) g_zoom--;
        UpdateMag();
    }
    break;

    case WM_MOUSEHWHEEL:
    {
        int d = GET_WHEEL_DELTA_WPARAM(wParam);  /**< Horizontal wheel */
        if (d > 0) g_zoom++; else if (g_zoom > 1) g_zoom--;
        UpdateMag();
    }
    break;

    case WM_KEYDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000)         /**< Ctrl */
        {
            if (wParam == VK_ADD || wParam == 0xBB)        /**< "+" */
                g_zoom++;
            else if ((wParam == VK_SUBTRACT || wParam == 0xBD) && g_zoom > 1)  /**< "-" */
                g_zoom--;
			else if (wParam == 'w' || wParam == 'W')    /**< "W" */
                g_followMouse = !g_followMouse;
                UpdateMag();
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
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInst, NULL);

    // Set the window icon to the defined application icon
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
    SetWindowPos(g_hMag, NULL, 0, 0, rc.right, rc.bottom - FOOTER_H, SWP_NOZORDER);

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
