#pragma once

#include "../util/ComPtr.hpp"
#include "../util/Geometry.hpp"

#include <d3d11.h>

#include <array>
#include <cstdint>
#include <optional>

namespace magshit::tools {

/// A pixel sample plus the desktop position it came from.
struct PickedColor
{
    std::uint8_t r, g, b;
    Point at;
};

/// Reads back a single pixel from a source `ID3D11Texture2D` using a 1x1
/// staging resource. Designed for per-frame use at low cost.
class Eyedropper
{
public:
    /// Lazily create the 1x1 staging texture. Idempotent. Returns false on
    /// device failure.
    bool ensureStaging(ID3D11Device* device);

    /// Sample one pixel from `source` at `texelPosWithinSource` and return
    /// the color, or std::nullopt if the position is out of bounds or
    /// readback failed. Side effect: updates `last()` and may push history.
    std::optional<PickedColor> sample(ID3D11Device* device,
                                      ID3D11DeviceContext* context,
                                      ID3D11Texture2D* source,
                                      Size sourceSize,
                                      Point desktopBoundsOrigin,
                                      Point texelPosWithinSource);

    /// Most recently sampled color, if any.
    const std::optional<PickedColor>& last() const noexcept { return last_; }

    /// Push `c` onto the small history ring (most-recent first).
    void pushHistory(const PickedColor& c);

    /// Fixed-size history ring (front == most recent).
    const std::array<PickedColor, 8>& history() const noexcept { return history_; }

    /// Number of valid entries in `history()` (0 .. 8).
    size_t historyCount() const noexcept { return histCount_; }

private:
    ComPtr<ID3D11Texture2D> staging_;
    std::optional<PickedColor> last_;
    std::array<PickedColor, 8> history_{};
    size_t histCount_ = 0;
};

} // namespace magshit::tools
