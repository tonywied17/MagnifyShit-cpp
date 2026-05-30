#pragma once

#include "../../util/Geometry.hpp"

namespace magshit::platform {

/**
 * @brief Perform one-time Windows process setup.
 */
void initProcess();

/**
 * @brief Query the union of all monitor bounds.
 * @return Virtual-screen rectangle in desktop coordinates.
 */
Rect virtualScreenBounds() noexcept;

/**
 * @brief Query the current cursor position.
 * @return Cursor position in virtual-screen coordinates.
 */
Point cursorPos() noexcept;

} // namespace magshit::platform
