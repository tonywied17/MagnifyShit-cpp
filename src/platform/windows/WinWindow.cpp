#include "WinWindow.hpp"

#include "../../util/Log.hpp"

#include <windowsx.h>

namespace magshit::platform {

WinWindow::WinWindow() : className_(L"MagnifyShitMainWnd") {}

WinWindow::~WinWindow()
{
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (inst_)
    {
        UnregisterClassW(className_.c_str(), inst_);
    }
}

bool WinWindow::create(HINSTANCE hInst, const WindowDesc& desc, MsgHandler handler)
{
    inst_ = hInst;
    handler_ = std::move(handler);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WinWindow::staticProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className_.c_str();
    if (!RegisterClassExW(&wc))
    {
        log::error("RegisterClassExW failed: {}", GetLastError());
        return false;
    }

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW,
                            className_.c_str(),
                            desc.title.c_str(),
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            desc.width,
                            desc.height,
                            nullptr,
                            nullptr,
                            hInst,
                            this);
    if (!hwnd_)
    {
        log::error("CreateWindowExW failed: {}", GetLastError());
        return false;
    }

    setAlwaysOnTop(topmost_);
    return true;
}

void WinWindow::show(int nCmdShow)
{
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
}

Size WinWindow::clientSize() const noexcept
{
    RECT r{};
    GetClientRect(hwnd_, &r);
    return Size{r.right - r.left, r.bottom - r.top};
}

Rect WinWindow::clientScreenRect() const noexcept
{
    RECT r{};
    GetClientRect(hwnd_, &r);
    POINT tl{r.left, r.top};
    ClientToScreen(hwnd_, &tl);
    return Rect{tl.x, tl.y, r.right - r.left, r.bottom - r.top};
}

void WinWindow::setBorderless(bool on)
{
    if (on == borderless_)
    {
        return;
    }
    borderless_ = on;
    const LONG style = GetWindowLong(hwnd_, GWL_STYLE);
    LONG newStyle = on ? (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP
                       : (style & ~WS_POPUP) | WS_OVERLAPPEDWINDOW;
    SetWindowLong(hwnd_, GWL_STYLE, newStyle);
    SetWindowPos(hwnd_,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void WinWindow::setAlwaysOnTop(bool on)
{
    topmost_ = on;
    SetWindowPos(hwnd_,
                 on ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void WinWindow::setClickThrough(bool on)
{
    if (on == clickThrough_)
    {
        return;
    }
    clickThrough_ = on;
    LONG ex = GetWindowLong(hwnd_, GWL_EXSTYLE);
    if (on) { ex |= WS_EX_LAYERED; }
    ex &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd_, GWL_EXSTYLE, ex);
    if (on)
    {
        SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
    }
}

void WinWindow::setTitle(const std::wstring& title)
{
    SetWindowTextW(hwnd_, title.c_str());
}

void WinWindow::setExcludedFromCapture(bool on)
{
    if (!hwnd_) return;
    excludedFromCapture_ = on;
    SetWindowDisplayAffinity(hwnd_, on ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
}

LRESULT CALLBACK WinWindow::staticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WinWindow* self = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<WinWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    else
    {
        self = reinterpret_cast<WinWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self)
    {
        return self->proc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WinWindow::proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (handler_)
    {
        bool handled = false;
        LRESULT r = handler_(hwnd, msg, wParam, lParam, handled);
        if (handled)
        {
            return r;
        }
    }
    switch (msg)
    {
    case WM_NCHITTEST:
        if (borderless_ && !IsZoomed(hwnd))
        {
            const LRESULT ht = hitTestBorderless(hwnd, lParam);
            if (ht != HTNOWHERE)
            {
                resizeHover_ = static_cast<int>(ht);
                return ht;
            }
        }
        resizeHover_ = HTNOWHERE;
        break;
    case WM_NCMOUSELEAVE:
    case WM_MOUSELEAVE:
        resizeHover_ = HTNOWHERE;
        break;
    case WM_GETMINMAXINFO:
    {
        // Keep the window large enough to remain usable in borderless mode
        // (the custom titlebar needs room for the drag region + buttons).
        const UINT dpi = GetDpiForWindow(hwnd) ? GetDpiForWindow(hwnd) : 96;
        const auto scale = [dpi](int v) { return MulDiv(v, dpi, 96); };
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = scale(640);
        mmi->ptMinTrackSize.y = scale(360);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WinWindow::hitTestBorderless(HWND hwnd, LPARAM lParam) const
{
    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    RECT wr{};
    GetWindowRect(hwnd, &wr);

    const UINT dpi = GetDpiForWindow(hwnd) ? GetDpiForWindow(hwnd) : 96;
    // Slightly wider grab strip than the visible decoration so corners are
    // forgiving without making interior clicks feel mushy.
    const int border = MulDiv(6, dpi, 96);
    const int corner = MulDiv(12, dpi, 96);

    const bool left   = pt.x < wr.left   + border;
    const bool right  = pt.x >= wr.right  - border;
    const bool top    = pt.y < wr.top    + border;
    const bool bottom = pt.y >= wr.bottom - border;

    // Corner zones win, with a larger pickup radius.
    const bool nearLeft   = pt.x < wr.left   + corner;
    const bool nearRight  = pt.x >= wr.right  - corner;
    const bool nearTop    = pt.y < wr.top    + corner;
    const bool nearBottom = pt.y >= wr.bottom - corner;

    if (top    && nearLeft)  return HTTOPLEFT;
    if (top    && nearRight) return HTTOPRIGHT;
    if (bottom && nearLeft)  return HTBOTTOMLEFT;
    if (bottom && nearRight) return HTBOTTOMRIGHT;
    if (left   && nearTop)   return HTTOPLEFT;
    if (left   && nearBottom)return HTBOTTOMLEFT;
    if (right  && nearTop)   return HTTOPRIGHT;
    if (right  && nearBottom)return HTBOTTOMRIGHT;

    if (left)   return HTLEFT;
    if (right)  return HTRIGHT;
    if (top)    return HTTOP;
    if (bottom) return HTBOTTOM;

    return HTNOWHERE;
}

} // namespace magshit::platform
