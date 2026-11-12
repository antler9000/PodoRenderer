struct InVertex
{
    float3 posL     : POSITION;
    float3 normalL  : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct OutVertex
{
    float4 posH     : SV_POSITION;
};

cbuffer Object : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvTransWorld;
}

cbuffer gDirectionalLights : register(b1)
{
    float       isShadowOn;
    float3      strength;
    float3      direction;
    float4x4    orthoViewProj;
}

void VS(in InVertex inVertex, out OutVertex outVertex)
{
    float4 posW = mul(float4(inVertex.posL, 1.0f), gWorld);
    
    outVertex.posH  = mul(posW, orthoViewProj);
}