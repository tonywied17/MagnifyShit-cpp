#pragma once

#include <algorithm>
#include <cstdint>

namespace magshit {

/// @brief 2D integer point.
struct Point
{
    std::int32_t x = 0;
    std::int32_t y = 0;
};

/// @brief 2D integer size (width, height).
struct Size
{
    std::int32_t w = 0;
    std::int32_t h = 0;
};

/// @brief 2D integer rectangle (top-left origin + size).
struct Rect
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t w = 0;
    std::int32_t h = 0;

    /**
     * @brief Compute the exclusive right edge.
     * @return One past the rightmost column.
     */
    constexpr std::int32_t right() const noexcept { return x + w; }

    /**
     * @brief Compute the exclusive bottom edge.
     * @return One past the bottom row.
     */
    constexpr std::int32_t bottom() const noexcept { return y + h; }

    /**
     * @brief Compute the rectangle center using integer rounding.
     * @return Center point in the same coordinate space as the rectangle.
     */
    constexpr Point center() const noexcept { return {x + w / 2, y + h / 2}; }
};

/**
 * @brief Clamp a rectangle so it fits fully inside another rectangle.
 * @param r Rectangle to clamp; width and height may be reduced.
 * @param bounds Container rectangle with positive area.
 * @return Adjusted rectangle fully contained in `bounds`.
 */
inline Rect clampToBounds(Rect r, Rect bounds) noexcept
{
    r.w = std::min(r.w, bounds.w);
    r.h = std::min(r.h, bounds.h);
    r.x = std::clamp(r.x, bounds.x, bounds.x + bounds.w - r.w);
    r.y = std::clamp(r.y, bounds.y, bounds.y + bounds.h - r.h);
    return r;
}

} // namespace magshit
