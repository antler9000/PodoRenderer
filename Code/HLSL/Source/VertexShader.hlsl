//NOTE: 프로젝트 구성의 HLSL 추가 포함 디렉토리에 Common 폴더를 추가해도, 상수가 미정의되었다는 거짓 보고가 발생해 상대 경로를 명시함
#include "../../Common/Common.h"

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

struct DirectionalLight
{
    float3 strength;
    float3 direction;
};

SamplerState gSamplerPointWrap      : register(s0);
SamplerState gSamplerLinearWrap     : register(s1);
SamplerState gSamplerAnisotropic    : register(s2);

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

cbuffer Light : register(b2)
{
    uint                gDirectionalLightsNum;
    DirectionalLight    gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
    float3              gAmbientLight;
}

cbuffer Camera : register(b3)
{
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