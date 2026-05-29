#pragma once

#include <algorithm>
#include <cstdint>

namespace magshit {

/// 2D integer point.
struct Point
{
    std::int32_t x = 0;
    std::int32_t y = 0;
};

/// 2D integer size (width, height).
struct Size
{
    std::int32_t w = 0;
    std::int32_t h = 0;
};

/// 2D integer rectangle (top-left origin + size).
struct Rect
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t w = 0;
    std::int32_t h = 0;

    /// One past the rightmost column.
    constexpr std::int32_t right() const noexcept { return x + w; }

    /// One past the bottom row.
    constexpr std::int32_t bottom() const noexcept { return y + h; }

    /// Center point of the rectangle (integer rounding).
    constexpr Point center() const noexcept { return {x + w / 2, y + h / 2}; }
};

/// Shrink `r` to fit inside `bounds` (width/height clamped, then position
/// clamped). The returned rect is always fully contained in `bounds`
/// assuming `bounds` has positive area.
inline Rect clampToBounds(Rect r, Rect bounds) noexcept
{
    r.w = std::min(r.w, bounds.w);
    r.h = std::min(r.h, bounds.h);
    r.x = std::clamp(r.x, bounds.x, bounds.x + bounds.w - r.w);
    r.y = std::clamp(r.y, bounds.y, bounds.y + bounds.h - r.h);
    return r;
}

} // namespace magshit
