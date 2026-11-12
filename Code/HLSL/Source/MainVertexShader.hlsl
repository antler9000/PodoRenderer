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
    bool        isShadowOn;
    float3      strength;
    float3      direction;
    float4x4    orthoViewProj;
};

SamplerState            gSamplerAnisotropic    : register(s0);
SamplerComparisonState  gSamplerComparison     : register(s1);

Texture2D gDiffuseMap : register(t0);

cbuffer Material : register(b0)
{
    float3  gDiffuseAlbedo;
    float3  gFresnelR0;
    float   gShininess;
}

cbuffer Object : register(b1)
{
    float4x4 gWorld;
    float4x4 gInvTransWorld;
}

Texture2D<float> gShadowMap[] : register(t0, space1);

ConstantBuffer<Light> gDirectionalLights[] : register(b0, space1);

cbuffer Camera : register(b2)
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