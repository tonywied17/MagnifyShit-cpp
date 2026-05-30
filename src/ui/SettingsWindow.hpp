#pragma once

#include "UiContext.hpp"

namespace magshit::ui {

/// @brief Draws the tabbed settings dialog.
class SettingsWindow
{
public:
    /**
     * @brief Build the settings window for the current frame when visible.
     * @param ctx Live UI context used to read and mutate application state.
     */
    void draw(UiContext& ctx);

private:
    bool wasShown_ = false;
};

} // namespace magshit::ui
