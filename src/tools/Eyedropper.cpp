#include "Eyedropper.hpp"

namespace magshit::tools {

bool Eyedropper::ensureStaging(ID3D11Device* device)
{
    if (staging_) return true;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc = {1, 0};
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    return SUCCEEDED(device->CreateTexture2D(&td, nullptr, staging_.GetAddressOf()));
}

std::optional<PickedColor> Eyedropper::sample(ID3D11Device* device,
                                              ID3D11DeviceContext* context,
                                              ID3D11Texture2D* source,
                                              Size sourceSize,
                                              Point desktopBoundsOrigin,
                                              Point texelPosWithinSource)
{
    if (!source || !ensureStaging(device))
    {
        return std::nullopt;
    }
    if (texelPosWithinSource.x < 0 || texelPosWithinSource.y < 0
        || texelPosWithinSource.x >= sourceSize.w || texelPosWithinSource.y >= sourceSize.h)
    {
        return last_;
    }

    D3D11_BOX box{};
    box.left = static_cast<UINT>(texelPosWithinSource.x);
    box.top = static_cast<UINT>(texelPosWithinSource.y);
    box.right = box.left + 1;
    box.bottom = box.top + 1;
    box.front = 0;
    box.back = 1;

    context->CopySubresourceRegion(staging_.Get(), 0, 0, 0, 0, source, 0, &box);

    D3D11_MAPPED_SUBRESOURCE map{};
    if (FAILED(context->Map(staging_.Get(), 0, D3D11_MAP_READ, 0, &map)))
    {
        return last_;
    }
    const auto* px = static_cast<const std::uint8_t*>(map.pData);
    PickedColor c{px[2], px[1], px[0],
                  Point{desktopBoundsOrigin.x + texelPosWithinSource.x,
                        desktopBoundsOrigin.y + texelPosWithinSource.y}};
    context->Unmap(staging_.Get(), 0);
    last_ = c;
    return c;
}

void Eyedropper::pushHistory(const PickedColor& c)
{
    for (size_t i = std::min(histCount_, history_.size() - 1); i > 0; --i)
    {
        history_[i] = history_[i - 1];
    }
    history_[0] = c;
    if (histCount_ < history_.size()) ++histCount_;
}

} // namespace magshit::tools
