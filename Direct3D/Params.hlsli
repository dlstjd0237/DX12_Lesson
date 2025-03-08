#ifndef _PARAMS_HLSLI_
#define _PARAMS_HLSLI_
#define MAXLIGHTS 16

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

struct Light
{
    int lightType; // 0 : Directional, 1 : Point, 2 : Spot
    float3 Strength;
    float FalloffStart;
    float3 Position;
    float FalloffEnd;
    float3 Direction;
    float SpotPower;
    float3 lightPadding;
};

cbuffer cbObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
}

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    int gTex_On;
    int gNormal_On;
    float2 gTexPadding;
}

cbuffer cbpass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    
    float4 gAmbientLight;
    float3 gEyePosW;
    int gLightCount;
    Light gLights[MAXLIGHTS];
    
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 gFogPadding;
}

TextureCube gCubeMap : register(t0);
Texture2D gTexture_0 : register(t1);
Texture2D gNormal_0 : register(t2);
Texture2DArray gTreeMapArray : register(t3);

SamplerState gSampler_0 : register(s0);

// normal map sample to world space
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW;
}

#endif