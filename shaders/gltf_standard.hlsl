#include "frame_constants.hlsl"

SamplerState s0 : register(s0);
Texture2D t0 : register(t0);

cbuffer NodeConstantBuffer : register(b1)
{
    float4x4 b1World : NODE_WORLD;
	float4 b1Color :MATERIAL_COLOR;
};

struct VSInput
{
    float3 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput vs)
{
    PSInput result;

    result.position = mul(b0Projection, mul(b0View, mul(b1World, float4(vs.position, 1))));
    result.normal = normalize(mul(b1World, float4(vs.normal, 0)).xyz);
    result.uv = vs.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 vColor = t0.Sample(s0, input.uv);
    float3 N = input.normal;
    float3 L = normalize(-b0LightDirection);
    float3 Shading = vColor.xyz * (saturate(dot(N, L)) + float3(0.2, 0.2, 0.2));
    return float4(Shading, 1) * b1Color;
}

technique MainTec0
{
    pass DrawObject
    {
        VertexShader = compile vs_3_0 VSMain();
        PixelShader = compile ps_3_0 PSMain();
    }
}
