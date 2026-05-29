#pragma once

#include "../../util/ComPtr.hpp"
#include "../../util/Geometry.hpp"

#include <d3d11.h>
#include <dxgi1_2.h>

namespace magshit::capture {

/// Wraps a single `IDXGIOutputDuplication` and the texture used to hand
/// captured frames to the renderer. Bound to one monitor at a time.
class DxgiDuplication
{
public:
    DxgiDuplication() = default;
    ~DxgiDuplication();

    DxgiDuplication(const DxgiDuplication&) = delete;
    DxgiDuplication& operator=(const DxgiDuplication&) = delete;

    /// Create the duplication against `monitor` (or the primary if null).
    /// Returns false if duplication could not be created.
    bool init(ID3D11Device* device, HMONITOR monitor = nullptr);

    /// Release all GPU resources. Safe to call multiple times.
    void shutdown();

    /// Switch to capturing `monitor`. Cheap if it's the same monitor we are
    /// already on. Returns false on failure (state left valid for the
    /// previous monitor).
    bool setMonitor(HMONITOR monitor);

    /// Monitor we are currently duplicating, or null if uninitialized.
    HMONITOR currentMonitor() const noexcept { return monitor_; }

    /// Pull the latest frame into the owned staging texture. Returns true
    /// if a new frame was captured; previous contents stay valid otherwise.
    bool acquire();

    /// Shader-resource view over the most recently captured frame.
    ID3D11ShaderResourceView* srv() const noexcept { return srv_.Get(); }

    /// Underlying texture (useful for CPU readback in the eyedropper).
    ID3D11Texture2D* sourceTexture() const noexcept { return staged_.Get(); }

    /// Texture dimensions (== source monitor pixels).
    Size textureSize() const noexcept { return texSize_; }

    /// Position and size of the source monitor in virtual-screen coords.
    Rect outputDesktopBounds() const noexcept { return desktopBounds_; }

    /// True once duplication is established and ready to acquire frames.
    bool valid() const noexcept { return duplication_ != nullptr; }

private:
    bool createForMonitor(HMONITOR monitor);
    bool createSurface();
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
