#pragma once

#include "../app/Application.hpp"
#include "../platform/windows/WinWindow.hpp"
#include "../tools/Eyedropper.hpp"
#include "Theme.hpp"

namespace magshit::ui {

/// @brief References to live objects the ImGui panels need during a frame.
struct UiContext
{
    app::AppState& state;
    platform::WinWindow& window;
    tools::Eyedropper* eyedropper = nullptr;
    app::HotkeyCapture* hotkeyCapture = nullptr;
    bool* hotkeyGlobalsDirty = nullptr; ///< set to true when a global binding changed
};

} // namespace magshit::ui
