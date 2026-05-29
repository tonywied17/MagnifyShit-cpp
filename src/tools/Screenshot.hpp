#pragma once

#include "../util/ComPtr.hpp"
#include "../util/Geometry.hpp"

#include <Windows.h>
#include <d3d11.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace magshit::tools {

/// Captures a BGRA8 `ID3D11Texture2D` (typically the swap-chain back
/// buffer), writes a PNG to disk, and optionally puts a DIB on the
/// clipboard.
class Screenshot
{
public:
    /// One screenshot pixel in the native BGRA8 layout used by the swap
    /// chain.
    struct Pixel
    {
        std::uint8_t b, g, r, a;
    };

    /// Capture `source`, write a PNG to `outDir` with a timestamped name,
    /// and optionally copy a DIB to the clipboard. If `pathOut` is non-null
    /// it is set to the file actually written. Returns false on any I/O or
    /// D3D failure.
    bool capture(ID3D11Device* device,
                 ID3D11DeviceContext* context,
                 ID3D11Texture2D* source,
                 const std::filesystem::path& outDir,
                 bool alsoCopyToClipboard,
                 HWND clipboardOwner,
                 std::filesystem::path* pathOut);

    /// Default output directory
    /// (`%USERPROFILE%\\Pictures\\MagnifyShit`), creating it if needed.
    static std::filesystem::path defaultOutputDir();

private:
    bool readback(ID3D11Device* device,
                  ID3D11DeviceContext* context,
                  ID3D11Texture2D* source,
                  Size& sizeOut,
                  std::vector<Pixel>& pixelsOut);

    bool writePng(const std::filesystem::path& path,
                  Size size,
                  const std::vector<Pixel>& pixels);

    bool copyToClipboard(HWND owner, Size size, const std::vector<Pixel>& pixels);
};

} // namespace magshit::tools
