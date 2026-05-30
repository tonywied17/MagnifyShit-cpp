#pragma once

#include "../../util/ComPtr.hpp"
#include "../../util/Geometry.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <cstdint>

namespace magshit::render {

/// @brief Up-sampling filter used to draw the magnified frame.
enum class Scaling : std::uint32_t
{
    Nearest = 0,
    Bilinear = 1,
    CatmullRom = 2,
    Lanczos3 = 3,
    _Count
};

/// @brief Bitmask used in `DrawParams::colorFlags`.
enum ColorFilterBits : std::uint32_t
{
    Filter_Invert    = 1u << 0,
    Filter_Grayscale = 1u << 1,
};

/// @brief Color-vision-deficiency simulation matrix selector.
enum class CvdMode : std::uint32_t
{
    None = 0,
    Protanopia = 1,
    Deuteranopia = 2,
    Tritanopia = 3,
};

/// @brief Per-frame inputs to `D3D11Renderer::drawMagnified`.
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

/// @brief Owns the D3D11 device, swap chain, and shader pipeline.
class D3D11Renderer
{
public:
    /**
     * @brief Construct an uninitialized renderer.
     */
    D3D11Renderer() = default;

    /**
     * @brief Release all D3D resources owned by the renderer.
     */
    ~D3D11Renderer();

    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    /**
     * @brief Create the device, swap chain, and pipeline state for a window.
     * @param hwnd Target window receiving swap-chain presentation.
     * @return true on success, false on a fatal D3D initialization failure.
     */
    bool init(HWND hwnd);

    /**
     * @brief Resize the swap-chain back buffer.
     * @param w New width in pixels.
     * @param h New height in pixels.
     */
    void resize(std::uint32_t w, std::uint32_t h);

    /**
     * @brief Clear and bind the current back buffer for drawing.
     * @param clearColor RGBA clear color.
     */
    void beginFrame(const float clearColor[4]);

    /**
     * @brief Draw a magnified, filtered fullscreen quad from a source texture.
     * @param srv Source shader-resource view to sample.
     * @param sourceTexSize Dimensions of `srv` in texels.
     * @param p Per-frame draw parameters and filter controls.
     */
    void drawMagnified(ID3D11ShaderResourceView* srv, Size sourceTexSize, const DrawParams& p);

    /**
     * @brief Present the current swap-chain back buffer.
     * @param vsync When true, wait for the next VBlank before returning.
     */
    void present(bool vsync);

    /**
     * @brief Access the D3D11 device.
     * @return Underlying device; never null after successful `init()`.
     */
    ID3D11Device* device() const noexcept { return device_.Get(); }

    /**
     * @brief Access the immediate D3D11 context.
     * @return Immediate context; never null after successful `init()`.
     */
    ID3D11DeviceContext* context() const noexcept { return context_.Get(); }

    /**
     * @brief Access the owned swap chain.
     * @return Swap chain; never null after successful `init()`.
     */
    IDXGISwapChain1* swapChain() const noexcept { return swapChain_.Get(); }

    /**
     * @brief Access the render-target view for the current back buffer.
     * @return Back-buffer render-target view, or null before initialization.
     */
    ID3D11RenderTargetView* backBufferRTV() const noexcept { return rtv_.Get(); }

    /**
     * @brief Query the current back-buffer size.
     * @return Back-buffer dimensions in pixels.
     */
    Size backBufferSize() const noexcept { return backSize_; }

private:
    /**
     * @brief Create the D3D11 device and immediate context.
     * @return true on success, false on D3D failure.
     */
    bool createDevice();

    /**
     * @brief Create the swap chain bound to a window.
     * @param hwnd Target presentation window.
     * @return true on success, false on DXGI failure.
     */
    bool createSwapChain(HWND hwnd);

    /**
     * @brief Compile shaders and create immutable pipeline state.
     * @return true on success, false on D3D shader or state creation failure.
     */
    bool createPipeline();

    /**
     * @brief Create a render-target view for the current swap-chain back buffer.
     * @return true on success, false on D3D failure.
     */
    bool createRTV();

    /**
     * @brief Release the current back-buffer render-target view.
     */
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
