#ifndef _TREE_HLSLI_
#define _TREE_HLSLI_

#include "Params.hlsli"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 Uv : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;
    
    return vout;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0;
    look = normalize(look);
    float3 right = cross(up, look);
    
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
    
    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);

    float2 texC[4] =
    {
        float2(0.0f, 1.0f),
      float2(0.0f, 0.0f),
      float2(1.0f, 1.0f),
      float2(1.0f, 0.0f)
    };

    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.Uv = texC[i];
        gout.PrimID = primID;
        
        triStream.Append(gout);
    }

}
float4 PS(GeoOut pin) : SV_Target
{
    float4 diffuseColor = gDiffuseAlbedo;

    float3 Uvw = float3(pin.Uv, pin.PrimID % 3);

    diffuseColor = gTreeMapArray.Sample(gSampler_0, Uvw) * gDiffuseAlbedo;

#ifdef ALPHA_TEST
    clip(diffuseColor.a - 0.1f);
#endif

    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float4 ambient = gAmbientLight * diffuseColor;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseColor, gFresnelR0, shininess };

    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, pin.NormalW, toEyeW);

    float4 litColor = ambient + directLight;

#ifdef FOG
    float distToEye = length(gEyePosW - pin.PosW);

    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    litColor.a = diffuseColor.a;

    return litColor;
}

#endif