#include "VertexShader.hlsl"

float4 PS(in OutVertex outVertex) : SV_TARGET
{   
    float3  normalW     = normalize(outVertex.normalW);
    float3  toEyeVec    = normalize(gEyePosW - outVertex.posW);
    
    const float microDistributionExponent       = gShininess * 256.0f;
    const float microDistributionNormalization  = (microDistributionExponent + 8.0f) / 8.0f;
    
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
        
        float3 lightStength = macroCosAngle * gDirectionalLights[i].strength;
        
        float3 reflectPercent = gFresnelR0 + (1.0f - gFresnelR0) * pow(1.0f - microCosAngle, 5.0f);
    
        float shininessFactor = pow(saturate(dot(halfVec, normalW)), microDistributionExponent) * microDistributionNormalization;
        
        specularLight   += lightStength * reflectPercent * shininessFactor;
        diffuseLight    += lightStength * sampledDiffuseAlbedo * gDiffuseAlbedo;
    }
    
    float3 finalLight = specularLight + diffuseLight + ambientLight;
    
    return float4(finalLight, 1);
}