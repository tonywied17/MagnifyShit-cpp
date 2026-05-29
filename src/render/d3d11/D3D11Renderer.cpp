#include "D3D11Renderer.hpp"

#include "../../util/Log.hpp"
#include "Shaders.hpp"

#include <d3dcompiler.h>
#include <dxgi1_3.h>

#include <cstring>

namespace magshit::render {

namespace {

struct alignas(16) ShaderCB
{
    float srcRect[4];
    float texSize[4];
    float fx[4];
    std::uint32_t colorFlags;
    std::uint32_t cvdMode;
    float gridOpacity;
    float zoom;
};

ComPtr<ID3DBlob> compile(const char* hlsl,
                         const char* entry,
                         const char* target,
                         const D3D_SHADER_MACRO* defines = nullptr)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errors;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = D3DCompile(hlsl,
                            std::strlen(hlsl),
                            nullptr,
                            defines,
                            nullptr,
                            entry,
                            target,
                            flags,
                            0,
                            blob.GetAddressOf(),
                            errors.GetAddressOf());
    if (FAILED(hr))
    {
        const char* msg = errors ? static_cast<const char*>(errors->GetBufferPointer())
                                 : "(no error blob)";
        log::error("Shader compile failed ({}): {}", entry, msg);
        return nullptr;
    }
    return blob;
}

const char* variantDefine(Scaling s)
{
    switch (s)
    {
    case Scaling::Nearest:    return "MAG_NEAREST";
    case Scaling::Bilinear:   return "MAG_BILINEAR";
    case Scaling::CatmullRom: return "MAG_CATMULL";
    case Scaling::Lanczos3:   return "MAG_LANCZOS";
    case Scaling::_Count:     break;
    }
    return "MAG_BILINEAR";
}

} // namespace

D3D11Renderer::~D3D11Renderer() = default;

bool D3D11Renderer::init(HWND hwnd)
{
    return createDevice() && createSwapChain(hwnd) && createRTV() && createPipeline();
}

bool D3D11Renderer::createDevice()
{
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT hr = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   flags,
                                   levels,
                                   _countof(levels),
                                   D3D11_SDK_VERSION,
                                   device_.GetAddressOf(),
                                   nullptr,
                                   context_.GetAddressOf());
    if (FAILED(hr))
    {
        log::error("D3D11CreateDevice failed: 0x{:08x}", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

bool D3D11Renderer::createSwapChain(HWND hwnd)
{
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device_.As(&dxgiDevice))) return false;
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(adapter.GetAddressOf()))) return false;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())))) return false;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    HRESULT hr = factory->CreateSwapChainForHwnd(
        device_.Get(), hwnd, &desc, nullptr, nullptr, swapChain_.GetAddressOf());
    if (FAILED(hr))
    {
        log::error("CreateSwapChainForHwnd failed: 0x{:08x}", static_cast<unsigned>(hr));
        return false;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    return true;
}

bool D3D11Renderer::createRTV()
{
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(back.GetAddressOf())))) return false;
    if (FAILED(device_->CreateRenderTargetView(back.Get(), nullptr, rtv_.GetAddressOf())))
        return false;
    D3D11_TEXTURE2D_DESC td{};
    back->GetDesc(&td);
    backSize_ = {static_cast<std::int32_t>(td.Width), static_cast<std::int32_t>(td.Height)};
    return true;
}

void D3D11Renderer::releaseRTV() { rtv_.Reset(); }

bool D3D11Renderer::createPipeline()
{
    auto vsBlob = compile(kFullscreenVS, "main", "vs_5_0");
    if (!vsBlob) return false;
    if (FAILED(device_->CreateVertexShader(vsBlob->GetBufferPointer(),
                                           vsBlob->GetBufferSize(),
                                           nullptr,
                                           vs_.GetAddressOf())))
        return false;

    for (size_t i = 0; i < psVariants_.size(); ++i)
    {
        D3D_SHADER_MACRO defs[] = {{variantDefine(static_cast<Scaling>(i)), "1"}, {nullptr, nullptr}};
        auto blob = compile(kSamplePS, "main", "ps_5_0", defs);
        if (!blob) return false;
        if (FAILED(device_->CreatePixelShader(blob->GetBufferPointer(),
                                              blob->GetBufferSize(),
                                              nullptr,
                                              psVariants_[i].GetAddressOf())))
            return false;
    }

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(ShaderCB);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device_->CreateBuffer(&bd, nullptr, cbuffer_.GetAddressOf()))) return false;

    D3D11_SAMPLER_DESC sd{};
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    if (FAILED(device_->CreateSamplerState(&sd, sampNearest_.GetAddressOf()))) return false;
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    if (FAILED(device_->CreateSamplerState(&sd, sampLinear_.GetAddressOf()))) return false;

    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    if (FAILED(device_->CreateRasterizerState(&rd, raster_.GetAddressOf()))) return false;

    D3D11_BLEND_DESC bld{};
    bld.RenderTarget[0].BlendEnable = FALSE;
    bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device_->CreateBlendState(&bld, blend_.GetAddressOf()))) return false;

    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = FALSE;
    if (FAILED(device_->CreateDepthStencilState(&dsd, depth_.GetAddressOf()))) return false;

    return true;
}

void D3D11Renderer::resize(std::uint32_t w, std::uint32_t h)
{
    if (!swapChain_ || w == 0 || h == 0) return;
    releaseRTV();
    swapChain_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    createRTV();
}

void D3D11Renderer::beginFrame(const float clearColor[4])
{
    context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(rtv_.Get(), clearColor);

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(backSize_.w);
    vp.Height = static_cast<float>(backSize_.h);
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);
}

void D3D11Renderer::drawMagnified(ID3D11ShaderResourceView* srv,
                                  Size sourceTexSize,
                                  const DrawParams& p)
{
    if (!srv) return;

    D3D11_MAPPED_SUBRESOURCE map{};
    if (SUCCEEDED(context_->Map(cbuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
    {
        auto* cb = static_cast<ShaderCB*>(map.pData);
        cb->srcRect[0] = p.uvX;
        cb->srcRect[1] = p.uvY;
        cb->srcRect[2] = p.uvW;
        cb->srcRect[3] = p.uvH;
        const float tw = static_cast<float>(sourceTexSize.w);
        const float th = static_cast<float>(sourceTexSize.h);
        cb->texSize[0] = tw;
        cb->texSize[1] = th;
        cb->texSize[2] = (tw > 0) ? 1.0f / tw : 0.0f;
        cb->texSize[3] = (th > 0) ? 1.0f / th : 0.0f;
        cb->fx[0] = p.brightness;
        cb->fx[1] = p.contrast;
        cb->fx[2] = p.gamma;
        cb->fx[3] = 0.0f;
        cb->colorFlags = p.colorFlags;
        cb->cvdMode = static_cast<std::uint32_t>(p.cvd);
        cb->gridOpacity = p.gridOpacity;
        cb->zoom = p.zoom;
        context_->Unmap(cbuffer_.Get(), 0);
    }

    auto idx = static_cast<size_t>(p.scaling);
    if (idx >= psVariants_.size()) idx = static_cast<size_t>(Scaling::Bilinear);

    ID3D11SamplerState* samps[] = {sampNearest_.Get(), sampLinear_.Get()};
    const float blendFactor[4] = {0, 0, 0, 0};
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context_->VSSetShader(vs_.Get(), nullptr, 0);
    context_->PSSetShader(psVariants_[idx].Get(), nullptr, 0);
    context_->PSSetConstantBuffers(0, 1, cbuffer_.GetAddressOf());
    context_->PSSetShaderResources(0, 1, &srv);
    context_->PSSetSamplers(0, 2, samps);
    context_->RSSetState(raster_.Get());
    context_->OMSetBlendState(blend_.Get(), blendFactor, 0xffffffff);
    context_->OMSetDepthStencilState(depth_.Get(), 0);
    context_->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    context_->PSSetShaderResources(0, 1, &nullSRV);
}

void D3D11Renderer::present(bool vsync) { swapChain_->Present(vsync ? 1u : 0u, 0); }

} // namespace magshit::render
