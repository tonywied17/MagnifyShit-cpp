#pragma once

#include "UiContext.hpp"

namespace magshit::ui {

/// Tabbed settings dialog (General, Filters, Appearance, Hotkeys, About).
/// Shown only when `AppState::showSettings` is true.
class SettingsWindow
{
public:
    /// Build the settings window for the current frame.
    void draw(UiContext& ctx);
};

} // namespace magshit::ui
