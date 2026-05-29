#pragma once

namespace magshit::render {

/// HLSL source compiled at runtime via `D3DCompile`. Pixel-shader variants
/// are produced by toggling `D3D_SHADER_MACRO` defines from
/// `D3D11Renderer.cpp`.

inline constexpr const char* kFullscreenVS = R"HLSL(
struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut main(uint vid : SV_VertexID)
{
    VSOut o;
    o.uv  = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}
)HLSL";

// Sample + post-process shader. Macros choose the scaling algorithm:
//   MAG_NEAREST | MAG_BILINEAR | MAG_CATMULL | MAG_LANCZOS
inline constexpr const char* kSamplePS = R"HLSL(
cbuffer CB : register(b0)
{
    float4 srcRect;     // xy = uv offset, zw = uv size
    float4 texSize;     // xy = texW,texH ; zw = 1/texW, 1/texH
    float4 fx;          // brightness, contrast, gamma, _unused
    uint   colorFlags;  // bit0 invert, bit1 grayscale
    uint   cvdMode;     // 0 none, 1 protan, 2 deutan, 3 tritan
    float  gridOpacity; // 0..1 pixel grid
    float  zoom;        // current zoom factor
};

Texture2D    src         : register(t0);
SamplerState sampNearest : register(s0);
SamplerState sampLinear  : register(s1);

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float3 srgb_to_lin(float3 c) { return pow(max(c, 0.0), 2.2); }
float3 lin_to_srgb(float3 c) { return pow(max(c, 0.0), 1.0 / 2.2); }

#if defined(MAG_CATMULL)
// 9-tap Catmull-Rom in two linear samples per axis (5 fetches total).
float4 sampleCatmullRom(float2 uv)
{
    float2 size = texSize.xy;
    float2 invSize = texSize.zw;

    float2 samplePos = uv * size;
    float2 texPos1   = floor(samplePos - 0.5) + 0.5;
    float2 f = samplePos - texPos1;

    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);

    float2 w12     = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    float2 texPos0  = (texPos1 - 1.0) * invSize;
    float2 texPos3  = (texPos1 + 2.0) * invSize;
    float2 texPos12 = (texPos1 + offset12) * invSize;

    float4 result = 0.0;
    result += src.SampleLevel(sampLinear, float2(texPos0.x,  texPos0.y),  0) * w0.x  * w0.y;
    result += src.SampleLevel(sampLinear, float2(texPos12.x, texPos0.y),  0) * w12.x * w0.y;
    result += src.SampleLevel(sampLinear, float2(texPos3.x,  texPos0.y),  0) * w3.x  * w0.y;

    result += src.SampleLevel(sampLinear, float2(texPos0.x,  texPos12.y), 0) * w0.x  * w12.y;
    result += src.SampleLevel(sampLinear, float2(texPos12.x, texPos12.y), 0) * w12.x * w12.y;
    result += src.SampleLevel(sampLinear, float2(texPos3.x,  texPos12.y), 0) * w3.x  * w12.y;

    result += src.SampleLevel(sampLinear, float2(texPos0.x,  texPos3.y),  0) * w0.x  * w3.y;
    result += src.SampleLevel(sampLinear, float2(texPos12.x, texPos3.y),  0) * w12.x * w3.y;
    result += src.SampleLevel(sampLinear, float2(texPos3.x,  texPos3.y),  0) * w3.x  * w3.y;
    return result;
}
#endif

#if defined(MAG_LANCZOS)
float sinc(float x)
{
    return (abs(x) < 1e-4) ? 1.0 : sin(3.14159265 * x) / (3.14159265 * x);
}
float lanczos3(float x)
{
    float a = 3.0;
    return (abs(x) < a) ? sinc(x) * sinc(x / a) : 0.0;
}
float4 sampleLanczos3(float2 uv)
{
    float2 size = texSize.xy;
    float2 invSize = texSize.zw;
    float2 samplePos = uv * size - 0.5;
    float2 base = floor(samplePos);
    float2 f = samplePos - base;

    float4 sum = 0.0;
    float wsum = 0.0;
    [unroll] for (int j = -2; j <= 3; ++j)
    {
        [unroll] for (int i = -2; i <= 3; ++i)
        {
            float wx = lanczos3(float(i) - f.x);
            float wy = lanczos3(float(j) - f.y);
            float w = wx * wy;
            float2 sUV = (base + float2(i, j) + 0.5) * invSize;
            sum  += src.SampleLevel(sampNearest, sUV, 0) * w;
            wsum += w;
        }
    }
    return sum / max(wsum, 1e-5);
}
#endif

float3 applyCVD(float3 c, uint mode)
{
    if (mode == 0u) return c;
    // Brettel/Vienot/Mollon simulation matrices (LMS -> projected -> RGB).
    float3x3 m;
    if (mode == 1u)      // Protanopia
        m = float3x3(0.567, 0.433, 0.000,
                     0.558, 0.442, 0.000,
                     0.000, 0.242, 0.758);
    else if (mode == 2u) // Deuteranopia
        m = float3x3(0.625, 0.375, 0.000,
                     0.700, 0.300, 0.000,
                     0.000, 0.300, 0.700);
    else                 // Tritanopia
        m = float3x3(0.950, 0.050, 0.000,
                     0.000, 0.433, 0.567,
                     0.000, 0.475, 0.525);
    return mul(m, c);
}

float4 main(VSOut i) : SV_Target
{
    float2 uv = srcRect.xy + i.uv * srcRect.zw;

    float4 c;
#if defined(MAG_NEAREST)
    c = src.Sample(sampNearest, uv);
#elif defined(MAG_BILINEAR)
    c = src.Sample(sampLinear, uv);
#elif defined(MAG_CATMULL)
    c = sampleCatmullRom(uv);
#elif defined(MAG_LANCZOS)
    c = sampleLanczos3(uv);
#else
    c = src.Sample(sampLinear, uv);
#endif

    float3 rgb = c.rgb;

    if ((colorFlags & 0x2u) != 0u)
    {
        float g = dot(rgb, float3(0.2126, 0.7152, 0.0722));
        rgb = float3(g, g, g);
    }
    if ((colorFlags & 0x1u) != 0u)
    {
        rgb = 1.0 - rgb;
    }

    rgb = applyCVD(rgb, cvdMode);

    rgb = (rgb - 0.5) * fx.y + 0.5 + fx.x;     // contrast + brightness
    rgb = pow(max(rgb, 0.0), 1.0 / max(fx.z, 0.01)); // gamma
    rgb = saturate(rgb);

    if (gridOpacity > 0.0)
    {
        float2 texel = uv * texSize.xy;
        float2 d = abs(frac(texel) - 0.5);
        float2 fw = fwidth(texel);
        float lineMask = 1.0 - smoothstep(0.0, max(fw.x, fw.y), min(d.x, d.y));
        float3 gridColor = float3(0.0, 0.0, 0.0);
        rgb = lerp(rgb, gridColor, lineMask * gridOpacity);
    }

    return float4(rgb, 1.0);
}
)HLSL";

} // namespace magshit::render
