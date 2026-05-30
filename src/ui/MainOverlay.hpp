#pragma once

#include "UiContext.hpp"

namespace magshit::ui {

/// @brief Draws the always-on-screen magnifier control panel.
class MainOverlay
{
public:
    /**
     * @brief Build the overlay window for the current frame.
     * @param ctx Live UI context used to read and mutate application state.
     */
    void draw(UiContext& ctx);
};

} // namespace magshit::ui
