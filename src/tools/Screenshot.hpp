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

/// @brief Captures BGRA8 textures to PNG files and optionally the clipboard.
class Screenshot
{
public:
    /// @brief One screenshot pixel in the native BGRA8 layout used by the swap chain.
    struct Pixel
    {
        std::uint8_t b, g, r, a;
    };

    /**
     * @brief Capture a texture to a timestamped PNG and optionally the clipboard.
     * @param device D3D11 device that owns `source`.
     * @param context Immediate context used for texture readback.
     * @param source BGRA8 texture to capture.
     * @param outDir Directory that receives the PNG file.
     * @param alsoCopyToClipboard true to copy a DIB to the clipboard after capture.
     * @param clipboardOwner Window used as the clipboard owner.
     * @param pathOut Optional destination for the PNG path actually written.
     * @return true on success; false on I/O, clipboard, or D3D failure.
     */
    bool capture(ID3D11Device* device,
                 ID3D11DeviceContext* context,
                 ID3D11Texture2D* source,
                 const std::filesystem::path& outDir,
                 bool alsoCopyToClipboard,
                 HWND clipboardOwner,
                 std::filesystem::path* pathOut);

    /**
     * @brief Resolve the default screenshot output directory.
     * @return `%USERPROFILE%\\Pictures\\MagnifyShit`, created if needed.
     */
    static std::filesystem::path defaultOutputDir();

private:
    /**
     * @brief Copy a GPU texture into CPU-readable memory.
     * @param device D3D11 device that owns `source`.
     * @param context Immediate context used for copy/map operations.
     * @param source Texture to read back.
     * @param sizeOut Receives source dimensions.
     * @param pixelsOut Receives BGRA8 pixels in row-major order.
     * @return true on successful readback; false on unsupported format or D3D failure.
     */
    bool readback(ID3D11Device* device,
                  ID3D11DeviceContext* context,
                  ID3D11Texture2D* source,
                  Size& sizeOut,
                  std::vector<Pixel>& pixelsOut);

    /**
     * @brief Write BGRA8 pixels to a PNG file.
     * @param path Destination PNG path.
     * @param size Image dimensions.
     * @param pixels BGRA8 pixels in row-major order.
     * @return true on success, false on encoder or I/O failure.
     */
    bool writePng(const std::filesystem::path& path,
                  Size size,
                  const std::vector<Pixel>& pixels);

    /**
     * @brief Copy BGRA8 pixels to the Windows clipboard as a DIB.
     * @param owner Window handle used as the clipboard owner.
     * @param size Image dimensions.
     * @param pixels BGRA8 pixels in row-major order.
     * @return true on success, false on clipboard allocation or ownership failure.
     */
    bool copyToClipboard(HWND owner, Size size, const std::vector<Pixel>& pixels);
};

} // namespace magshit::tools
