#pragma once

#include "../../util/Geometry.hpp"

namespace magshit::platform {

/// One-time process setup: per-monitor DPI awareness, COM, etc. Call once
/// at startup before creating windows.
void initProcess();

/// Returns the union of all monitor bounds in virtual-screen coordinates.
Rect virtualScreenBounds() noexcept;

/// Returns the cursor position in virtual-screen coordinates.
Point cursorPos() noexcept;

} // namespace magshit::platform
