#ifndef _DEFULT_HLSLI_
#define _DEFULT_HLSLI_


#include "Params.hlsli"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Uv : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float3 PosW : POSITION;
    float4 PosH : SV_Position;
    float3 NormalW : COLOR;
    float3 TangentW : TANGENT;
    float2 Uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    float4 Uv = mul(float4(vin.Uv, 0.0f, 1.0f), gTexTransform);
    vout.Uv = Uv.xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseColor = gDiffuseAlbedo;
    
    if (gTex_On)
    {
        diffuseColor = gTexture_0.Sample(gSampler_0, pin.Uv) * gDiffuseAlbedo;
    }
    
#ifdef ALPHA_TEST
    clip(diffuseColor.a - 0.1f);
#endif
    
    pin.NormalW = normalize(pin.NormalW);
    
    float4 normalMapSample;
    if (gNormal_On)
    {
        normalMapSample = gNormal_0.Sample(gSampler_0, pin.Uv);
        pin.NormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    }
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    float4 ambient = gAmbientLight * diffuseColor;
    
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseColor, gFresnelR0, shininess };
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, pin.NormalW, toEyeW);
    
    float4 litColor = ambient + directLight;
    
    float3 r = reflect(-toEyeW, pin.NormalW);
    float4 reflectionColor = gCubeMap.Sample(gSampler_0, r);
    float3 fresnelFactor = SchlickFresnel(gFresnelR0, pin.NormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
    
    
#ifdef FOG
    float distToEye = length(gEyePosW - pin.PosW);
    
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif
    
    litColor.a = diffuseColor.a;
    
    return litColor;
}
#endif