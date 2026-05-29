#include "WinPlatform.hpp"

#include <Windows.h>
#include <ShellScalingApi.h>

namespace magshit::platform {

void initProcess()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

Rect virtualScreenBounds() noexcept
{
    return Rect{
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN),
    };
}

Point cursorPos() noexcept
{
    POINT p{};
    GetCursorPos(&p);
    return Point{p.x, p.y};
}

} // namespace magshit::platform
