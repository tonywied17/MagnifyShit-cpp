#include "Application.hpp"

#include "Config.hpp"
#include "../platform/windows/WinPlatform.hpp"
#include "../ui/ImGuiBackend.hpp"
#include "../ui/MainOverlay.hpp"
#include "../ui/SettingsWindow.hpp"
#include "../ui/Theme.hpp"
#include "../ui/UiContext.hpp"
#include "../util/Geometry.hpp"
#include "../util/Log.hpp"

#include <ShellScalingApi.h>
#include <imgui.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

namespace magshit::app {

namespace {
constexpr UINT_PTR kDragRedrawTimer = 1;
constexpr int     kHotkeyBaseId     = 0x4000;

/**
 * @brief Convert the app's modifier bitmask to Win32 hotkey flags.
 * @param mods Modifier bitmask using `ModBits`.
 * @return Win32 `MOD_*` flag mask including `MOD_NOREPEAT`.
 */
UINT modsToWin32(std::uint8_t mods)
{
    UINT m = MOD_NOREPEAT;
    if (mods & Mod_Ctrl)  m |= MOD_CONTROL;
    if (mods & Mod_Shift) m |= MOD_SHIFT;
    if (mods & Mod_Alt)   m |= MOD_ALT;
    if (mods & Mod_Win)   m |= MOD_WIN;
    return m;
}
} // namespace

Application::Application() = default;
Application::~Application() { shutdown(); }

int Application::run(HINSTANCE hInst, int nCmdShow)
{
    if (!initialize(hInst, nCmdShow))
    {
        shutdown();
        return -1;
    }

    MSG msg{};
    while (true)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (msg.message == WM_QUIT)
        {
            break;
        }
        frame();
    }

    shutdown();
    return static_cast<int>(msg.wParam);
}

bool Application::initialize(HINSTANCE hInst, int nCmdShow)
{
    platform::initProcess();

    configPath_ = Config::defaultPath();
    Config::load(configPath_, state_);

    window_ = std::make_unique<platform::WinWindow>();
    if (!window_->create(hInst,
                         platform::WindowDesc{L"MagnifyShit 2.1", 1400, 820},
                         [this](HWND h, UINT m, WPARAM w, LPARAM l, bool& handled) {
                             return handleMessage(h, m, w, l, handled);
                         }))
    {
        return false;
    }

    renderer_ = std::make_unique<render::D3D11Renderer>();
    if (!renderer_->init(window_->handle()))
    {
        return false;
    }

    capture_ = std::make_unique<capture::DxgiDuplication>();
    if (!capture_->init(renderer_->device()))
    {
        log::warn("Capture init failed; magnifier will show a clear color.");
    }

    imgui_ = std::make_unique<ui::ImGuiBackend>();
    if (!imgui_->init(window_->handle(), renderer_->device(), renderer_->context()))
    {
        return false;
    }
    imgui_->setThemeMode(state_.themeMode);
    const UINT dpi = GetDpiForWindow(window_->handle());
    imgui_->rebuildFonts(static_cast<float>(dpi) / 96.0f);

    overlay_ = std::make_unique<ui::MainOverlay>();
    settings_ = std::make_unique<ui::SettingsWindow>();
    eyedropper_ = std::make_unique<tools::Eyedropper>();
    screenshot_ = std::make_unique<tools::Screenshot>();

    window_->show(nCmdShow);
    window_->setAlwaysOnTop(state_.alwaysOnTop);
    window_->setExcludedFromCapture(state_.excludeFromCapture);
    window_->setClickThrough(state_.clickThrough);
    if (state_.borderless != window_->borderless())
    {
        window_->setBorderless(state_.borderless);
    }

    rebindGlobalHotkeys();

    updateTitle();
    return true;
}

void Application::rebindGlobalHotkeys()
{
    if (!window_ || !window_->handle()) return;
    HWND const hwnd = window_->handle();
    for (int i = 0; i < static_cast<int>(HotkeyAction::_Count); ++i)
    {
        UnregisterHotKey(hwnd, kHotkeyBaseId + i);
    }
    for (const auto& info : hotkeyActionInfos())
    {
        if (!info.global) continue;
        const auto& binds = state_.hotkeys.bindings[static_cast<size_t>(info.action)];
        for (const auto& b : binds)
        {
            if (b.trigger != HotkeyTrigger::Key) continue;
            RegisterHotKey(hwnd,
                           kHotkeyBaseId + static_cast<int>(info.action),
                           modsToWin32(b.mods),
                           b.vk);
            break; // one OS registration per global action
        }
    }
}

void Application::shutdown()
{
    if (window_ && window_->handle())
    {
        for (int i = 0; i < static_cast<int>(HotkeyAction::_Count); ++i)
        {
            UnregisterHotKey(window_->handle(), kHotkeyBaseId + i);
        }
    }
    if (!configPath_.empty())
    {
        Config::save(configPath_, state_);
    }
    screenshot_.reset();
    eyedropper_.reset();
    settings_.reset();
    overlay_.reset();
    imgui_.reset();
    capture_.reset();
    renderer_.reset();
    window_.reset();
}

void Application::frame()
{
    const bool uiWantsMouse = imgui_ && imgui_->ready() && ImGui::GetIO().WantCaptureMouse;
    if (state_.mode == MagnifierMode::AttachToMouse && !uiWantsMouse)
    {
        updateAttachedWindowPosition();
    }

    if (capture_ && capture_->valid())
    {
        HMONITOR target = nullptr;
        if (state_.mode == MagnifierMode::Static)
        {
            target = MonitorFromWindow(window_->handle(), MONITOR_DEFAULTTONEAREST);
        }
        else
        {
            POINT cp{};
            GetCursorPos(&cp);
            target = MonitorFromPoint(cp, MONITOR_DEFAULTTONEAREST);
        }
        if (target && target != capture_->currentMonitor())
        {
            capture_->setMonitor(target);
        }
    }

    if (capture_ && !state_.freeze)
    {
        capture_->acquire();
    }

    const float clear[4] = {0.06f, 0.06f, 0.07f, 1.0f};
    renderer_->beginFrame(clear);

    if (capture_ && capture_->valid())
    {
        const Size client = window_->clientSize();
        const Rect bounds = capture_->outputDesktopBounds();
        const Size tex = capture_->textureSize();

        const float zoom = std::max(0.01f, state_.zoom);
        const int srcW = std::max(1, static_cast<int>(client.w / zoom));
        const int srcH = std::max(1, static_cast<int>(client.h / zoom));

        Point center{};
        if (state_.mode == MagnifierMode::Static)
        {
            center = window_->clientScreenRect().center();
        }
        else
        {
            center = platform::cursorPos();
        }

        Rect src{center.x - srcW / 2, center.y - srcH / 2, srcW, srcH};
        src = clampToBounds(src, bounds);

        lastSrcDesktop_ = src;
        lastClient_ = client;
        lastFrameValid_ = true;

        const float uvX = static_cast<float>(src.x - bounds.x) / static_cast<float>(tex.w);
        const float uvY = static_cast<float>(src.y - bounds.y) / static_cast<float>(tex.h);
        const float uvW = static_cast<float>(src.w) / static_cast<float>(tex.w);
        const float uvH = static_cast<float>(src.h) / static_cast<float>(tex.h);

        render::DrawParams dp;
        dp.uvX = uvX;
        dp.uvY = uvY;
        dp.uvW = uvW;
        dp.uvH = uvH;
        dp.scaling = state_.scaling;
        if (state_.pixelPerfectSnap)
        {
            const float nearestInt = std::round(zoom);
            if (std::abs(zoom - nearestInt) < 0.05f && nearestInt >= 1.0f)
            {
                dp.scaling = render::Scaling::Nearest;
            }
        }
        dp.colorFlags = (state_.invert ? render::Filter_Invert : 0u)
                        | (state_.grayscale ? render::Filter_Grayscale : 0u);
        dp.cvd = state_.cvd;
        dp.brightness = state_.brightness;
        dp.contrast = state_.contrast;
        dp.gamma = state_.gamma;
        const bool showGrid = state_.gridAuto ? (zoom >= state_.gridThreshold) : state_.gridOn;
        dp.gridOpacity = showGrid ? state_.gridOpacity : 0.0f;
        dp.zoom = zoom;

        renderer_->drawMagnified(capture_->srv(), tex, dp);

        if (state_.showEyedropper && eyedropper_)
        {
            const Point cur = platform::cursorPos();
            const Point tp{cur.x - bounds.x, cur.y - bounds.y};
            eyedropper_->sample(renderer_->device(),
                                renderer_->context(),
                                capture_->sourceTexture(),
                                tex,
                                Point{bounds.x, bounds.y},
                                tp);
        }
    }

    renderUi();
    renderer_->present(true);
}

void Application::updateAttachedWindowPosition()
{
    POINT pt;
    GetCursorPos(&pt);
    RECT rc;
    GetWindowRect(window_->handle(), &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    SetWindowPos(window_->handle(),
                 nullptr,
                 pt.x - w / 2,
                 pt.y - h / 2,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
}

void Application::renderUi()
{
    if (!imgui_ || !imgui_->ready())
    {
        return;
    }

    imgui_->beginFrame();
    if (imgui_->themeMode() != state_.themeMode)
    {
        imgui_->setThemeMode(state_.themeMode);
    }
    const float prevZoom = state_.zoom;
    bool globalsDirty = false;
    ui::UiContext ctx{state_, *window_, eyedropper_.get(), &capture_state_, &globalsDirty};
    overlay_->draw(ctx);
    settings_->draw(ctx);
    if (globalsDirty)
    {
        rebindGlobalHotkeys();
    }
    capture_state_.justCaptured = false;
    if (state_.zoom != prevZoom)
    {
        updateTitle();
    }
    imgui_->endFrame();
}

LRESULT Application::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, bool& handled)
{
    if (imgui_ && imgui_->handleMessage(hwnd, msg, wParam, lParam))
    {
        handled = true;
        return 1;
    }

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && renderer_)
        {
            renderer_->resize(LOWORD(lParam), HIWORD(lParam));
        }
        handled = true;
        return 0;
    case WM_ENTERSIZEMOVE:
        SetTimer(hwnd, kDragRedrawTimer, 16, nullptr);
        handled = true;
        return 0;
    case WM_EXITSIZEMOVE:
        KillTimer(hwnd, kDragRedrawTimer);
        if (imgui_ && imgui_->ready())
        {
            // The OS move/size modal loop swallows WM_LBUTTONUP; without
            // this ImGui keeps the drag InvisibleButton "active" so the
            // next mouse-down is ignored until the user clicks again.
            ImGui::GetIO().AddMouseButtonEvent(0, false);
        }
        handled = true;
        return 0;
    case WM_TIMER:
        if (wParam == kDragRedrawTimer)
        {
            frame();
            handled = true;
            return 0;
        }
        break;
    case WM_HOTKEY:
    {
        const int id = static_cast<int>(wParam) - kHotkeyBaseId;
        if (id >= 0 && id < static_cast<int>(HotkeyAction::_Count))
        {
            runAction(static_cast<HotkeyAction>(id));
            handled = true;
            return 0;
        }
        break;
    }
    case WM_DPICHANGED:
    {
        const RECT* r = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hwnd,
                     nullptr,
                     r->left,
                     r->top,
                     r->right - r->left,
                     r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        const UINT dpi = HIWORD(wParam);
        if (imgui_)
        {
            imgui_->rebuildFonts(static_cast<float>(dpi) / 96.0f);
        }
        handled = true;
        return 0;
    }
    case WM_KEYDOWN:
        onKeyDown(wParam);
        handled = true;
        return 0;
    case WM_MOUSEWHEEL:
        onWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        handled = true;
        return 0;
    case WM_MOUSEMOVE:
    {
        const bool uiHasMouse =
            imgui_ && imgui_->ready() && ImGui::GetIO().WantCaptureMouse;
        if (state_.clickThrough && ctDragging_ && !uiHasMouse)
        {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            forwardMouseEvent(pt, msg, wParam);
            handled = true;
            return 0;
        }
        break;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        const bool uiHasMouse =
            imgui_ && imgui_->ready() && ImGui::GetIO().WantCaptureMouse;
        if (state_.clickThrough && !uiHasMouse)
        {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            forwardMouseEvent(pt, msg, wParam);
            handled = true;
            return 0;
        }
        if (msg == WM_LBUTTONDOWN)
        {
            onLeftClick();
        }
        handled = true;
        return 0;
    }
    case WM_NCHITTEST:
        // The custom overlay titlebar provides explicit drag; we no longer
        // treat the whole client as a caption (that swallowed clicks in
        // borderless + click-through mode).
        break;
    case WM_CAPTURECHANGED:
        ctDragging_ = false;
        break;
    default: break;
    }
    return 0;
}

void Application::onKeyDown(WPARAM key)
{
    const std::uint16_t vk = static_cast<std::uint16_t>(key);
    const std::uint8_t mods = currentModifierMask();

    if (capture_state_.active)
    {
        if (vk == VK_ESCAPE)
        {
            capture_state_.active = false;
            return;
        }
        if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU ||
            vk == VK_LWIN || vk == VK_RWIN)
        {
            return; // wait for the non-modifier key
        }
        capture_state_.captured = HotkeyBinding{mods, HotkeyTrigger::Key, vk};
        capture_state_.justCaptured = true;
        capture_state_.active = false;
        return;
    }

    if (vk == VK_ESCAPE)
    {
        if (state_.clickThrough)
        {
            state_.clickThrough = false;
            window_->setClickThrough(false);
            if (ctDragging_) { ReleaseCapture(); ctDragging_ = false; }
            return;
        }
        if (state_.mode == MagnifierMode::AttachToMouse)
        {
            state_.mode = MagnifierMode::Static;
            return;
        }
        if (window_->borderless())
        {
            toggleBorderless();
        }
        return;
    }

    const HotkeyAction action = state_.hotkeys.matchKey(mods, vk);
    if (action != HotkeyAction::_Count)
    {
        if (hotkeyActionInfo(action).global)
        {
            return; // already handled via WM_HOTKEY
        }
        runAction(action);
    }
}

void Application::onWheel(short delta)
{
    const std::uint8_t mods = currentModifierMask();

    if (capture_state_.active && mods != 0)
    {
        capture_state_.captured = HotkeyBinding{
            mods,
            delta > 0 ? HotkeyTrigger::WheelUp : HotkeyTrigger::WheelDown,
            0,
        };
        capture_state_.justCaptured = true;
        capture_state_.active = false;
        return;
    }

    const HotkeyAction action = state_.hotkeys.matchWheel(mods, delta);
    if (action != HotkeyAction::_Count)
    {
        runAction(action);
    }
}

void Application::runAction(HotkeyAction action)
{
    switch (action)
    {
    case HotkeyAction::ToggleOverlay:    state_.showOverlay = !state_.showOverlay; break;
    case HotkeyAction::ToggleBorderless: toggleBorderless(); break;
    case HotkeyAction::FreezeToggle:     state_.freeze = !state_.freeze; break;
    case HotkeyAction::ZoomIn:           adjustZoom(+0.1f); break;
    case HotkeyAction::ZoomOut:          adjustZoom(-0.1f); break;
    case HotkeyAction::ResetZoom:        resetZoom(); break;
    case HotkeyAction::CycleMode:        cycleMode(); break;
    case HotkeyAction::AttachToggle:
        state_.mode = (state_.mode == MagnifierMode::AttachToMouse) ? MagnifierMode::Static
                                                                    : MagnifierMode::AttachToMouse;
        if (state_.mode == MagnifierMode::AttachToMouse) state_.showOverlay = false;
        break;
    case HotkeyAction::AlwaysOnTopToggle:
        state_.alwaysOnTop = !state_.alwaysOnTop;
        window_->setAlwaysOnTop(state_.alwaysOnTop);
        break;
    case HotkeyAction::ClickThroughToggle:
        state_.clickThrough = !state_.clickThrough;
        window_->setClickThrough(state_.clickThrough);
        if (ctDragging_) { ReleaseCapture(); ctDragging_ = false; }
        break;
    case HotkeyAction::EyedropperToggle: state_.showEyedropper = !state_.showEyedropper; break;
    case HotkeyAction::Screenshot:       takeScreenshot(); break;
    case HotkeyAction::OpenSettings:     state_.showSettings = !state_.showSettings; break;
    case HotkeyAction::CopyHexAtCursor:  copyHexAtCursor(); break;
    case HotkeyAction::Quit:             PostMessageW(window_->handle(), WM_CLOSE, 0, 0); break;
    case HotkeyAction::_Count: break;
    }
}

bool Application::forwardMouseEvent(POINT clientPt, UINT msg, WPARAM /*wParam*/)
{
    if (!lastFrameValid_ || lastClient_.w <= 0 || lastClient_.h <= 0)
    {
        return false;
    }

    clientPt.x = std::clamp<LONG>(clientPt.x, 0, lastClient_.w - 1);
    clientPt.y = std::clamp<LONG>(clientPt.y, 0, lastClient_.h - 1);

    const double u = (static_cast<double>(clientPt.x) + 0.5) /
                     static_cast<double>(lastClient_.w);
    const double v = (static_cast<double>(clientPt.y) + 0.5) /
                     static_cast<double>(lastClient_.h);
    const int desktopX = lastSrcDesktop_.x + static_cast<int>(
        std::floor(u * static_cast<double>(lastSrcDesktop_.w)));
    const int desktopY = lastSrcDesktop_.y + static_cast<int>(
        std::floor(v * static_cast<double>(lastSrcDesktop_.h)));

    HWND const hwnd = window_->handle();
    LONG const exOrig = GetWindowLongW(hwnd, GWL_EXSTYLE);
    SetWindowLongW(hwnd, GWL_EXSTYLE, exOrig | WS_EX_TRANSPARENT | WS_EX_LAYERED);

    POINT desktopPt{desktopX, desktopY};
    HWND target = WindowFromPoint(desktopPt);
    if (target)
    {
        POINT cli = desktopPt;
        ScreenToClient(target, &cli);
        HWND child = ChildWindowFromPointEx(target, cli,
                                            CWP_SKIPINVISIBLE | CWP_SKIPDISABLED |
                                                CWP_SKIPTRANSPARENT);
        if (child && child != target) target = child;
    }

    SetWindowLongW(hwnd, GWL_EXSTYLE, exOrig);

    if (!target || target == hwnd)
    {
        return false;
    }

    POINT cli = desktopPt;
    ScreenToClient(target, &cli);

    WPARAM wp = 0;
    switch (msg)
    {
    case WM_LBUTTONDOWN: wp = MK_LBUTTON; break;
    case WM_RBUTTONDOWN: wp = MK_RBUTTON; break;
    case WM_MBUTTONDOWN: wp = MK_MBUTTON; break;
    case WM_MOUSEMOVE:
        if (GetKeyState(VK_LBUTTON) & 0x8000) wp |= MK_LBUTTON;
        if (GetKeyState(VK_RBUTTON) & 0x8000) wp |= MK_RBUTTON;
        if (GetKeyState(VK_MBUTTON) & 0x8000) wp |= MK_MBUTTON;
        break;
    default: break;
    }

    PostMessageW(target, msg, wp, MAKELPARAM(cli.x, cli.y));

    const bool isDown = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
                         msg == WM_MBUTTONDOWN);
    const bool isUp   = (msg == WM_LBUTTONUP   || msg == WM_RBUTTONUP   ||
                         msg == WM_MBUTTONUP);
    if (isDown)
    {
        SetCapture(hwnd);
        ctDragging_  = true;
        ctDragMoved_ = false;
    }
    else if (isUp && ctDragging_)
    {
        ReleaseCapture();
        ctDragging_  = false;
        ctDragMoved_ = false;
    }
    else if (msg == WM_MOUSEMOVE && ctDragging_)
    {
        ctDragMoved_ = true;
    }
    return true;
}

void Application::onLeftClick() {}

void Application::copyHexAtCursor()
{
    if (!capture_ || !capture_->valid() || !eyedropper_ || !renderer_)
    {
        return;
    }
    POINT cp{};
    GetCursorPos(&cp);
    HMONITOR const mon = MonitorFromPoint(cp, MONITOR_DEFAULTTONEAREST);
    if (mon && mon != capture_->currentMonitor())
    {
        capture_->setMonitor(mon);
    }
    capture_->acquire();

    const Rect bounds = capture_->outputDesktopBounds();
    const Size tex = capture_->textureSize();
    const Point tp{cp.x - bounds.x, cp.y - bounds.y};
    auto picked = eyedropper_->sample(renderer_->device(), renderer_->context(),
                                      capture_->sourceTexture(), tex,
                                      Point{bounds.x, bounds.y}, tp);
    if (!picked) return;

    char hex[16];
    std::snprintf(hex, sizeof(hex), "#%02X%02X%02X",
                  picked->r, picked->g, picked->b);

    bool copied = false;
    if (OpenClipboard(window_->handle()))
    {
        EmptyClipboard();
        const size_t bytes = std::strlen(hex) + 1;
        if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes); h != nullptr)
        {
            if (void* p = GlobalLock(h))
            {
                std::memcpy(p, hex, bytes);
                GlobalUnlock(h);
                if (SetClipboardData(CF_TEXT, h)) copied = true;
            }
            else
            {
                GlobalFree(h);
            }
        }
        CloseClipboard();
    }

    if (copied)
    {
        state_.toastText = std::string("Copied ") + hex;
        state_.toastUntilTime = ImGui::GetTime() + 1.8;
    }
}

void Application::adjustZoom(float delta)
{
    state_.zoom = std::clamp(state_.zoom + delta, state_.zoomMin, state_.zoomMax);
    updateTitle();
}

void Application::resetZoom()
{
    state_.zoom = 1.0f;
    updateTitle();
}

void Application::cycleMode()
{
    switch (state_.mode)
    {
    case MagnifierMode::Static:        state_.mode = MagnifierMode::FollowMouse;   break;
    case MagnifierMode::FollowMouse:   state_.mode = MagnifierMode::AttachToMouse; break;
    case MagnifierMode::AttachToMouse: state_.mode = MagnifierMode::Static;        break;
    }
    // Attached mode follows the cursor; the overlay would lag behind and block view.
    if (state_.mode == MagnifierMode::AttachToMouse) state_.showOverlay = false;
}

void Application::toggleBorderless()
{
    window_->setBorderless(!window_->borderless());
    state_.borderless = window_->borderless();
}

void Application::updateTitle()
{
    window_->setTitle(std::format(L"MagnifyShit 2.1 \u2014 {:.2f}x", state_.zoom));
}

void Application::takeScreenshot()
{
    if (!renderer_ || !screenshot_)
    {
        return;
    }
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(renderer_->swapChain()->GetBuffer(0, IID_PPV_ARGS(back.GetAddressOf()))))
    {
        log::warn("Screenshot: GetBuffer failed");
        return;
    }
    std::filesystem::path out;
    if (screenshot_->capture(renderer_->device(),
                             renderer_->context(),
                             back.Get(),
                             tools::Screenshot::defaultOutputDir(),
                             true,
                             window_->handle(),
                             &out))
    {
        state_.lastScreenshotPath = out.string();
        log::info("Screenshot saved: {}", state_.lastScreenshotPath);
    }
}

} // namespace magshit::app
