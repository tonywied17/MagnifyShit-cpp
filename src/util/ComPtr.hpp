#pragma once

#include <wrl/client.h>

namespace magshit {
/// Alias for `Microsoft::WRL::ComPtr<T>`. Used pervasively for COM
/// interfaces (D3D11/DXGI).
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
} // namespace magshit
