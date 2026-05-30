#pragma once

#include "../../util/ComPtr.hpp"
#include "../../util/Geometry.hpp"

#include <d3d11.h>
#include <dxgi1_2.h>

namespace magshit::capture {

/// @brief Wraps `IDXGIOutputDuplication` capture for one monitor at a time.
class DxgiDuplication
{
public:
    /**
     * @brief Construct an uninitialized capture session.
     */
    DxgiDuplication() = default;

    /**
     * @brief Release duplication and GPU resources.
     */
    ~DxgiDuplication();

    DxgiDuplication(const DxgiDuplication&) = delete;
    DxgiDuplication& operator=(const DxgiDuplication&) = delete;

    /**
     * @brief Create desktop duplication for a monitor.
     * @param device D3D11 device used for duplication resources.
     * @param monitor Monitor to capture, or null for the primary monitor.
     * @return true when duplication is ready; false on DXGI or D3D failure.
     */
    bool init(ID3D11Device* device, HMONITOR monitor = nullptr);

    /**
     * @brief Release all GPU resources.
     */
    void shutdown();

    /**
     * @brief Switch capture to another monitor.
     * @param monitor Monitor to capture, or null for the primary monitor.
     * @return true on success; false leaves the previous monitor state intact.
     */
    bool setMonitor(HMONITOR monitor);

    /**
     * @brief Query the currently duplicated monitor.
     * @return Current monitor handle, or null if uninitialized.
     */
    HMONITOR currentMonitor() const noexcept { return monitor_; }

    /**
     * @brief Pull the latest desktop frame into the owned texture.
     * @return true if a new frame was captured; false leaves previous contents valid.
     */
    bool acquire();

    /**
     * @brief Access the shader-resource view for the latest captured frame.
     * @return SRV over the capture texture, or null before successful initialization.
     */
    ID3D11ShaderResourceView* srv() const noexcept { return srv_.Get(); }

    /**
     * @brief Access the underlying captured texture.
     * @return Texture containing the latest frame, or null before initialization.
     */
    ID3D11Texture2D* sourceTexture() const noexcept { return staged_.Get(); }

    /**
     * @brief Query capture texture dimensions.
     * @return Texture size in source-monitor pixels.
     */
    Size textureSize() const noexcept { return texSize_; }

    /**
     * @brief Query the source monitor rectangle.
     * @return Monitor bounds in virtual-screen coordinates.
     */
    Rect outputDesktopBounds() const noexcept { return desktopBounds_; }

    /**
     * @brief Check whether duplication is established.
     * @return true once ready to acquire frames.
     */
    bool valid() const noexcept { return duplication_ != nullptr; }

private:
    /**
     * @brief Create duplication objects for a monitor.
     * @param monitor Monitor to capture, or null for the primary monitor.
     * @return true on success, false on DXGI or D3D failure.
     */
    bool createForMonitor(HMONITOR monitor);

    /**
     * @brief Create the texture and shader-resource view backing capture output.
     * @return true on success, false on D3D failure.
     */
    bool createSurface();

    /**
     * @brief Release the capture texture and shader-resource view.
     */
    void releaseSurface();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGIOutputDuplication> duplication_;
    ComPtr<ID3D11Texture2D> staged_;
    ComPtr<ID3D11ShaderResourceView> srv_;
    Size texSize_{};
    Rect desktopBounds_{};
    HMONITOR monitor_ = nullptr;
};

} // namespace magshit::capture
