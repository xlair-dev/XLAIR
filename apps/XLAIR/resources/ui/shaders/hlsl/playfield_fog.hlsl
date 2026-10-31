// Based on OpenSiv3D's forward3d.hlsl (MIT License).
// Copyright (c) 2008-2025 Ryo Suzuki, 2016-2025 OpenSiv3D Project.
Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

namespace s3d {
    struct PSInput {
        float4 position : SV_POSITION;
        float3 worldPosition : TEXCOORD0;
        float2 uv : TEXCOORD1;
    };
}

cbuffer PSPerFrame : register(b0) {
    float3 g_globalAmbientColor;
    float3 g_sunColor;
    float3 g_sunDirection;
}

cbuffer PSPerMaterial : register(b3) {
    float3 g_ambientColor;
    uint g_hasTexture;
    float4 g_diffuseColor;
    // Preserve Siv3D's material-buffer layout and the emission offset.
    float3 g_specularColor;
    float g_shininess;
    float3 g_emissionColor;
}

cbuffer PSFog : register(b4) {
    float4 g_fogColorAndStart;
    float4 g_fogEndAndStrength;
}

float4 PS(s3d::PSInput input) : SV_TARGET {
    float4 diffuseColor = g_diffuseColor;
    if (g_hasTexture) {
        diffuseColor *= g_texture0.Sample(g_sampler0, input.uv);
    }

    // The playfield uses ambient-only lighting (sun color is always zero).
    float3 ambientColor = g_ambientColor * g_globalAmbientColor;
    float3 surfaceColor = ambientColor * diffuseColor.rgb + g_emissionColor;

    float fog = smoothstep(g_fogColorAndStart.w, g_fogEndAndStrength.x, input.worldPosition.z);
    fog *= g_fogEndAndStrength.y;
    return float4(lerp(surfaceColor, g_fogColorAndStart.rgb, fog), diffuseColor.a);
}
