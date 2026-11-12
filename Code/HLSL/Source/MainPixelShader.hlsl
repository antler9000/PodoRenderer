#include "MainVertexShader.hlsl"

float3 CalculateRecievingStrength(float macroCosAngle, float3 lightStrength)
{
    float3 lambertStrength = macroCosAngle * lightStrength;
    
    return lambertStrength;
}

float CalculateShadowFactor(bool isShadowOn, int lightIndex, float3 posW)
{
    if(isShadowOn == false)
    {
        return 1.0f;
    }
    
    float3 shadowNDC = mul(float4(posW, 1.0f), gDirectionalLights[lightIndex].orthoViewProj).xyz;
    float2 shadowUV = float2(shadowNDC.x / 2 + 0.5f, -shadowNDC.y / 2 + 0.5f);
    float shadowFactor = gShadowMap[lightIndex].SampleCmpLevelZero(gSamplerComparison, shadowUV, shadowNDC.z);
    
    return shadowFactor;
}

float3 CalculateReflectPercent(float microCosAngle)
{
    float3 reflectPercent = gFresnelR0 + (1.0f - gFresnelR0) * pow(1.0f - microCosAngle, 5.0f);
    
    return reflectPercent;
}

float3 CalculateShininessFactor(float3 halfVec, float3 normalW, float microDistributionExponent, float microDistributionNormalization)
{
    float3 shininessFactor = pow(saturate(dot(halfVec, normalW)), microDistributionExponent) * microDistributionNormalization;

    return shininessFactor;
}

float4 PS(in OutVertex outVertex) : SV_TARGET
{   
    float3  normalW     = normalize(outVertex.normalW);
    float3  toEyeVec    = normalize(gEyePosW - outVertex.posW);
    
    float microDistributionExponent         = gShininess * 256.0f;
    float microDistributionNormalization    = (microDistributionExponent + 8.0f) / 8.0f;
    
    float3 sampledDiffuseAlbedo = gDiffuseMap.Sample(gSamplerAnisotropic, outVertex.texCoord).xyz;
    
    float3 specularLight    = float3(0.0f, 0.0f, 0.0f);
    float3 diffuseLight     = float3(0.0f, 0.0f, 0.0f);
    float3 ambientLight     = gAmbientLightStrength * sampledDiffuseAlbedo * gDiffuseAlbedo;
    for (uint i = 0; i < gDirectionalLightsNum; i++)
    {
        float3  lightVec        = normalize(-1 * gDirectionalLights[i].direction);
        float3  halfVec         = normalize(toEyeVec + lightVec);
        float   macroCosAngle   = saturate(dot(normalW, lightVec));
        float   microCosAngle   = saturate(dot(halfVec, lightVec));
          
        float3  recievingStength    = CalculateRecievingStrength(macroCosAngle, gDirectionalLights[i].strength);
        float   shadowFactor        = CalculateShadowFactor(gDirectionalLights[i].isShadowOn, i, outVertex.posW);
        float3  reflectPercent      = CalculateReflectPercent(microCosAngle);
        float3  shininessFactor     = CalculateShininessFactor(halfVec, normalW, microDistributionExponent, microDistributionNormalization);
        
        specularLight   += recievingStength * shadowFactor * reflectPercent * shininessFactor;
        diffuseLight    += recievingStength * shadowFactor * sampledDiffuseAlbedo * gDiffuseAlbedo;
    }
    
    float3 finalLight = specularLight + diffuseLight + ambientLight;
    
    return float4(finalLight, 1);
}