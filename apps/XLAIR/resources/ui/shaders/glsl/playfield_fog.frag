// Based on OpenSiv3D's forward3d.frag (MIT License).
// Copyright (c) 2008-2025 Ryo Suzuki, 2016-2025 OpenSiv3D Project.
#version 410

uniform sampler2D Texture0;

layout(location = 0) in vec3 WorldPosition;
layout(location = 1) in vec2 UV;

layout(location = 0) out vec4 FragColor;

layout(std140) uniform PSPerFrame {
    vec3 g_globalAmbientColor;
    vec3 g_sunColor;
    vec3 g_sunDirection;
};

layout(std140) uniform PSPerMaterial {
    vec3 g_ambientColor;
    uint g_hasTexture;
    vec4 g_diffuseColor;
    // Preserve Siv3D's material-buffer layout and the emission offset.
    vec3 g_specularColor;
    float g_shininess;
    vec3 g_emissionColor;
};

layout(std140) uniform PSFog {
    vec4 g_fogColorAndStart;
    vec4 g_fogEndAndStrength;
};

void main() {
    vec4 diffuseColor = g_diffuseColor;
    if (g_hasTexture == 1u) {
        diffuseColor *= texture(Texture0, UV);
    }

    // The playfield uses ambient-only lighting (sun color is always zero).
    vec3 ambientColor = g_ambientColor * g_globalAmbientColor;
    vec3 surfaceColor = ambientColor * diffuseColor.rgb + g_emissionColor;

    float fog = smoothstep(g_fogColorAndStart.w, g_fogEndAndStrength.x, WorldPosition.z);
    fog *= g_fogEndAndStrength.y;
    FragColor = vec4(mix(surfaceColor, g_fogColorAndStart.rgb, fog), diffuseColor.a);
}
