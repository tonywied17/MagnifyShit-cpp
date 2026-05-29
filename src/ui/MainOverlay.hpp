#pragma once

#include "UiContext.hpp"

namespace magshit::ui {

/// The always-on-screen control panel: zoom slider, mode dropdown, window
/// flags, eyedropper read-out, and the shortcuts cheat-sheet.
class MainOverlay
{
public:
    /// Build the overlay window for the current frame.
    void draw(UiContext& ctx);
};

} // namespace magshit::ui
