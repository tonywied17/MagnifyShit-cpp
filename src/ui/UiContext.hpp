#pragma once

#include "../app/Application.hpp"
#include "../platform/windows/WinWindow.hpp"
#include "../tools/Eyedropper.hpp"
#include "Theme.hpp"

namespace magshit::ui {

/// References to the live application objects the ImGui panels need to
/// read or mutate during a frame.
struct UiContext
{
    app::AppState& state;
    platform::WinWindow& window;
    tools::Eyedropper* eyedropper = nullptr;
    app::HotkeyCapture* hotkeyCapture = nullptr;
    bool* hotkeyGlobalsDirty = nullptr; ///< set to true when a global binding changed
};

} // namespace magshit::ui
