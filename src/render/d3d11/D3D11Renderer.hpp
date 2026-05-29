#pragma once

#include "../../util/ComPtr.hpp"
#include "../../util/Geometry.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <cstdint>

namespace magshit::render {

/// Up-sampling filter used to draw the magnified frame.
enum class Scaling : std::uint32_t
{
    Nearest = 0,
    Bilinear = 1,
    CatmullRom = 2,
    Lanczos3 = 3,
    _Count
};

/// Bitmask used in `DrawParams::colorFlags`.
enum ColorFilterBits : std::uint32_t
{
    Filter_Invert    = 1u << 0,
    Filter_Grayscale = 1u << 1,
};

/// Color-vision-deficiency simulation matrix selector.
enum class CvdMode : std::uint32_t
{
    None = 0,
    Protanopia = 1,
    Deuteranopia = 2,
    Tritanopia = 3,
};

/// Per-frame inputs to `D3D11Renderer::drawMagnified`.
struct DrawParams
{
    float uvX = 0.0f;
    float uvY = 0.0f;
    float uvW = 1.0f;
    float uvH = 1.0f;
    Scaling scaling = Scaling::Bilinear;
    std::uint32_t colorFlags = 0;
    CvdMode cvd = CvdMode::None;
    float brightness = 0.0f;
    float contrast = 1.0f;
    float gamma = 1.0f;
    float gridOpacity = 0.0f;
    float zoom = 1.0f;
};

/// Owns the D3D11 device, swap chain, and shader pipeline. Draws a single
/// magnified quad per frame on top of a cleared back buffer.
class D3D11Renderer
{
public:
    D3D11Renderer() = default;
    ~D3D11Renderer();

    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    /// Create device, swap chain, and pipeline state for `hwnd`. Returns
    /// false on a fatal D3D failure.
    bool init(HWND hwnd);

    /// Resize the swap chain back buffer to `w` x `h` pixels.
    void resize(std::uint32_t w, std::uint32_t h);

    /// Clear the back buffer to `clearColor` and bind it as the render
    /// target. Call once at the top of each frame.
    void beginFrame(const float clearColor[4]);

    /// Draw the source SRV as a magnified, filtered quad over the back
    /// buffer using `p`.
    void drawMagnified(ID3D11ShaderResourceView* srv, Size sourceTexSize, const DrawParams& p);

    /// Submit the frame. `vsync = true` waits for the next VBlank.
    void present(bool vsync);

    /// Underlying D3D11 device (never null after a successful `init`).
    ID3D11Device* device() const noexcept { return device_.Get(); }

    /// Immediate context.
    ID3D11DeviceContext* context() const noexcept { return context_.Get(); }

    /// Owned swap chain.
    IDXGISwapChain1* swapChain() const noexcept { return swapChain_.Get(); }

    /// Render-target view for the current back buffer.
    ID3D11RenderTargetView* backBufferRTV() const noexcept { return rtv_.Get(); }

    /// Back buffer size in pixels.
    Size backBufferSize() const noexcept { return backSize_; }

private:
    bool createDevice();
    bool createSwapChain(HWND hwnd);
    bool createPipeline();
    bool createRTV();
    void releaseRTV();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain1> swapChain_;
    ComPtr<ID3D11RenderTargetView> rtv_;
    ComPtr<ID3D11VertexShader> vs_;
    std::array<ComPtr<ID3D11PixelShader>, static_cast<size_t>(Scaling::_Count)> psVariants_;
    ComPtr<ID3D11Buffer> cbuffer_;
    ComPtr<ID3D11SamplerState> sampNearest_;
    ComPtr<ID3D11SamplerState> sampLinear_;
    ComPtr<ID3D11RasterizerState> raster_;
    ComPtr<ID3D11BlendState> blend_;
    ComPtr<ID3D11DepthStencilState> depth_;
    Size backSize_{};
};

} // namespace magshit::render
