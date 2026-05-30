#include "Screenshot.hpp"

#include "../util/Log.hpp"

#include <ShlObj.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <chrono>
#include <format>
#include <vector>

namespace magshit::tools {

namespace {

/**
 * @brief Build a timestamped PNG filename for a screenshot.
 * @return Filename in the form `magshit_YYYYMMDD_HHMMSS.png`.
 */
std::wstring timestampName()
{
    using namespace std::chrono;
    const auto now = system_clock::to_time_t(system_clock::now());
    tm local{};
    localtime_s(&local, &now);
    return std::format(L"magshit_{:04}{:02}{:02}_{:02}{:02}{:02}.png",
                       local.tm_year + 1900,
                       local.tm_mon + 1,
                       local.tm_mday,
                       local.tm_hour,
                       local.tm_min,
                       local.tm_sec);
}

} // namespace

std::filesystem::path Screenshot::defaultOutputDir()
{
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &p)))
    {
        std::filesystem::path dir(p);
        CoTaskMemFree(p);
        dir /= L"MagnifyShit";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return dir;
    }
    return std::filesystem::current_path();
}

bool Screenshot::readback(ID3D11Device* device,
                          ID3D11DeviceContext* context,
                          ID3D11Texture2D* source,
                          Size& sizeOut,
                          std::vector<Pixel>& pixelsOut)
{
    if (!source) return false;

    D3D11_TEXTURE2D_DESC sd{};
    source->GetDesc(&sd);

    D3D11_TEXTURE2D_DESC td = sd;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.SampleDesc = {1, 0};
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    td.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device->CreateTexture2D(&td, nullptr, staging.GetAddressOf())))
    {
        log::error("Screenshot: failed to create staging texture");
        return false;
    }

    context->CopyResource(staging.Get(), source);

    D3D11_MAPPED_SUBRESOURCE map{};
    if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map)))
    {
        log::error("Screenshot: Map staging failed");
        return false;
    }

    sizeOut = {static_cast<std::int32_t>(sd.Width), static_cast<std::int32_t>(sd.Height)};
    pixelsOut.resize(static_cast<size_t>(sd.Width) * sd.Height);

    const auto* src = static_cast<const std::uint8_t*>(map.pData);
    for (UINT y = 0; y < sd.Height; ++y)
    {
        std::memcpy(&pixelsOut[y * sd.Width], src + y * map.RowPitch, sd.Width * sizeof(Pixel));
    }
    context->Unmap(staging.Get(), 0);
    return true;
}

bool Screenshot::writePng(const std::filesystem::path& path,
                          Size size,
                          const std::vector<Pixel>& pixels)
{
    std::vector<std::uint8_t> rgba(static_cast<size_t>(size.w) * size.h * 4);
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        rgba[i * 4 + 0] = pixels[i].r;
        rgba[i * 4 + 1] = pixels[i].g;
        rgba[i * 4 + 2] = pixels[i].b;
        rgba[i * 4 + 3] = 0xFF;
    }
    const std::string utf8 = path.string();
    int ok = stbi_write_png(
        utf8.c_str(), size.w, size.h, 4, rgba.data(), size.w * 4);
    if (!ok)
    {
        log::error("Screenshot: stbi_write_png failed for {}", utf8);
        return false;
    }
    return true;
}

bool Screenshot::copyToClipboard(HWND owner, Size size, const std::vector<Pixel>& pixels)
{
    if (!OpenClipboard(owner))
    {
        return false;
    }
    EmptyClipboard();

    const size_t pixelBytes = static_cast<size_t>(size.w) * size.h * 4;
    const size_t total = sizeof(BITMAPV5HEADER) + pixelBytes;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, total);
    if (!h)
    {
        CloseClipboard();
        return false;
    }

    auto* hdr = static_cast<BITMAPV5HEADER*>(GlobalLock(h));
    ZeroMemory(hdr, sizeof(BITMAPV5HEADER));
    hdr->bV5Size = sizeof(BITMAPV5HEADER);
    hdr->bV5Width = size.w;
    hdr->bV5Height = -size.h;
    hdr->bV5Planes = 1;
    hdr->bV5BitCount = 32;
    hdr->bV5Compression = BI_BITFIELDS;
    hdr->bV5SizeImage = static_cast<DWORD>(pixelBytes);
    hdr->bV5RedMask   = 0x00FF0000;
    hdr->bV5GreenMask = 0x0000FF00;
    hdr->bV5BlueMask  = 0x000000FF;
    hdr->bV5AlphaMask = 0xFF000000;
    hdr->bV5CSType = LCS_WINDOWS_COLOR_SPACE;
    hdr->bV5Intent = LCS_GM_IMAGES;

    auto* dst = reinterpret_cast<std::uint8_t*>(hdr) + sizeof(BITMAPV5HEADER);
    std::memcpy(dst, pixels.data(), pixelBytes);
    GlobalUnlock(h);

    SetClipboardData(CF_DIBV5, h);
    CloseClipboard();
    return true;
}

bool Screenshot::capture(ID3D11Device* device,
                         ID3D11DeviceContext* context,
                         ID3D11Texture2D* source,
                         const std::filesystem::path& outDir,
                         bool alsoCopyToClipboard,
                         HWND clipboardOwner,
                         std::filesystem::path* pathOut)
{
    Size size{};
    std::vector<Pixel> pixels;
    if (!readback(device, context, source, size, pixels))
    {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    const auto path = outDir / timestampName();
    const bool wroteFile = writePng(path, size, pixels);
    const bool wroteClip = alsoCopyToClipboard && copyToClipboard(clipboardOwner, size, pixels);

    if (pathOut && wroteFile)
    {
        *pathOut = path;
    }
    return wroteFile || wroteClip;
}

} // namespace magshit::tools
