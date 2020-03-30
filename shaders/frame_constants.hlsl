cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View : CAMERA_VIEW;
    float4x4 b0Projection : CAMERA_PROJECTION;
    float3 b0LightDirection : LIGHT_DIRECTION;
    float3 b0LightColor : LIGHT_COLOR;
};
