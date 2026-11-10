struct InVertex
{
    float3 posL     : POSITION;
    float3 normalL  : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct OutVertex
{
    float3 posW     : POSITION;
    float4 posH     : SV_POSITION;
    float3 normalW  : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct Light
{
    float3      strength;
    float3      direction;
    float4x4    orthoViewPorj;
};

SamplerState gSamplerPointWrap      : register(s0);
SamplerState gSamplerLinearWrap     : register(s1);
SamplerState gSamplerAnisotropic    : register(s2);

Texture2D gDiffuseMap : register(t0, space0);

cbuffer Material : register(b0, space0)
{
    float3  gDiffuseAlbedo;
    float3  gFresnelR0;
    float   gShininess;
}

cbuffer Object : register(b1, space0)
{
    float4x4 gWorld;
    float4x4 gInvTransWorld;
}

ConstantBuffer<Light> gDirectionalLights[] : register(b0, space1);

cbuffer Camera : register(b2, space0)
{
    uint        gDirectionalLightsNum;
    float3      gAmbientLightStrength;
    float3      gEyePosW;
    float4x4    gViewProj;
}

void VS(in InVertex inVertex, out OutVertex outVertex)
{
    outVertex.posW = mul(float4(inVertex.posL, 1.0f), gWorld).xyz;
    outVertex.posH = mul(float4(outVertex.posW, 1.0f), gViewProj);
    
    outVertex.normalW = mul(float4(inVertex.normalL, 0.0f), gInvTransWorld).xyz;
    
    outVertex.texCoord = inVertex.texCoord;
}