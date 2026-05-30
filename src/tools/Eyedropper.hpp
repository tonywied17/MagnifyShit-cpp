#pragma once

#include "../util/ComPtr.hpp"
#include "../util/Geometry.hpp"

#include <d3d11.h>

#include <array>
#include <cstdint>
#include <optional>

namespace magshit::tools {

/// @brief A pixel sample plus the desktop position it came from.
struct PickedColor
{
    std::uint8_t r, g, b;
    Point at;
};

/// @brief Reads back individual pixels from a D3D11 texture.
class Eyedropper
{
public:
    /**
     * @brief Lazily create the 1x1 staging texture used for readback.
     * @param device D3D11 device that owns the staging texture.
     * @return true when the staging resource is ready; false on D3D failure.
     */
    bool ensureStaging(ID3D11Device* device);

    /**
     * @brief Sample one pixel from a source texture.
     * @param device D3D11 device that owns `source`.
     * @param context Immediate context used to copy/map the staging texture.
     * @param source Texture to sample.
     * @param sourceSize Dimensions of `source` in texels.
     * @param desktopBoundsOrigin Desktop-space origin of `source`.
     * @param texelPosWithinSource Texel position to sample inside `source`.
     * @return Picked color with desktop position, or `std::nullopt` on bounds or readback failure.
     */
    std::optional<PickedColor> sample(ID3D11Device* device,
                                      ID3D11DeviceContext* context,
                                      ID3D11Texture2D* source,
                                      Size sourceSize,
                                      Point desktopBoundsOrigin,
                                      Point texelPosWithinSource);

    /**
     * @brief Query the most recently sampled color.
     * @return Optional color sample, empty before the first successful sample.
     */
    const std::optional<PickedColor>& last() const noexcept { return last_; }

    /**
     * @brief Push a color sample onto the history ring.
     * @param c Sample to store as the most recent entry.
     */
    void pushHistory(const PickedColor& c);

    /**
     * @brief Access the fixed-size history ring.
     * @return Array of samples with index 0 as most recent.
     */
    const std::array<PickedColor, 8>& history() const noexcept { return history_; }

    /**
     * @brief Query the number of valid history entries.
     * @return Count in the range 0..8.
     */
    size_t historyCount() const noexcept { return histCount_; }

private:
    ComPtr<ID3D11Texture2D> staging_;
    std::optional<PickedColor> last_;
    std::array<PickedColor, 8> history_{};
    size_t histCount_ = 0;
};

} // namespace magshit::tools
