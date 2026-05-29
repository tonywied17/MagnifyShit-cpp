#include "DxgiDuplication.hpp"

#include "../../util/Log.hpp"

#include <dxgi1_2.h>

namespace magshit::capture {

DxgiDuplication::~DxgiDuplication()
{
    shutdown();
}

bool DxgiDuplication::init(ID3D11Device* device, HMONITOR monitor)
{
    device_ = device;
    device_->GetImmediateContext(context_.GetAddressOf());
    return createForMonitor(monitor);
}

bool DxgiDuplication::setMonitor(HMONITOR monitor)
{
    if (monitor == monitor_ && duplication_)
    {
        return true;
    }
    duplication_.Reset();
    releaseSurface();
    return createForMonitor(monitor);
}

void DxgiDuplication::shutdown()
{
    releaseSurface();
    duplication_.Reset();
    context_.Reset();
    device_.Reset();
}

bool DxgiDuplication::createForMonitor(HMONITOR monitor)
{
    monitor_ = monitor;
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device_.As(&dxgiDevice)))
    {
        return false;
    }
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(adapter.GetAddressOf())))
    {
        return false;
    }

    ComPtr<IDXGIOutput> chosenOutput;
    UINT i = 0;
    ComPtr<IDXGIOutput> output;
    while (adapter->EnumOutputs(i++, output.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc{};
        output->GetDesc(&desc);
        if (monitor == nullptr || desc.Monitor == monitor)
        {
            chosenOutput = output;
            desktopBounds_ = Rect{
                desc.DesktopCoordinates.left,
                desc.DesktopCoordinates.top,
                desc.DesktopCoordinates.right - desc.DesktopCoordinates.left,
                desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top,
            };
            monitor_ = desc.Monitor;
            if (monitor != nullptr)
            {
                break;
            }
            if (desktopBounds_.x == 0 && desktopBounds_.y == 0)
            {
                break;
            }
        }
    }
    if (!chosenOutput)
    {
        log::error("DXGI: no output found for monitor");
        return false;
    }

    ComPtr<IDXGIOutput1> output1;
    if (FAILED(chosenOutput.As(&output1)))
    {
        return false;
    }
    HRESULT hr = output1->DuplicateOutput(device_.Get(), duplication_.GetAddressOf());
    if (FAILED(hr))
    {
        log::error("IDXGIOutput1::DuplicateOutput failed: 0x{:08x}", static_cast<unsigned>(hr));
        return false;
    }

    DXGI_OUTDUPL_DESC od{};
    duplication_->GetDesc(&od);
    texSize_ = {static_cast<std::int32_t>(od.ModeDesc.Width),
                static_cast<std::int32_t>(od.ModeDesc.Height)};

    return createSurface();
}

bool DxgiDuplication::createSurface()
{
    releaseSurface();

    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(texSize_.w);
    td.Height = static_cast<UINT>(texSize_.h);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device_->CreateTexture2D(&td, nullptr, staged_.GetAddressOf())))
    {
        return false;
    }
    return SUCCEEDED(device_->CreateShaderResourceView(staged_.Get(), nullptr, srv_.GetAddressOf()));
}

void DxgiDuplication::releaseSurface()
{
    srv_.Reset();
    staged_.Reset();
}

bool DxgiDuplication::acquire()
{
    if (!duplication_)
    {
        return false;
    }

    DXGI_OUTDUPL_FRAME_INFO info{};
    ComPtr<IDXGIResource> resource;
    HRESULT hr = duplication_->AcquireNextFrame(0, &info, resource.GetAddressOf());

    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        return false;
    }
    if (hr == DXGI_ERROR_ACCESS_LOST)
    {
        log::warn("DXGI: access lost, reinitializing duplication");
        duplication_.Reset();
        createForMonitor(monitor_);
        return false;
    }
    if (FAILED(hr))
    {
        log::warn("DXGI: AcquireNextFrame failed 0x{:08x}", static_cast<unsigned>(hr));
        return false;
    }

    ComPtr<ID3D11Texture2D> frame;
    if (SUCCEEDED(resource.As(&frame)))
    {
        context_->CopyResource(staged_.Get(), frame.Get());
    }
    duplication_->ReleaseFrame();
    return true;
}

} // namespace magshit::capture
