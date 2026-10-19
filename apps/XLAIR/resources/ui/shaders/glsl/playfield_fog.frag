// Based on OpenSiv3D's forward3d.frag (MIT License).
// Copyright (c) 2008-2025 Ryo Suzuki, 2016-2025 OpenSiv3D Project.
#version 410

uniform sampler2D Texture0;

layout(location = 0) in vec3 WorldPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec4 FragColor;

layout(std140) uniform PSPerFrame {
    vec3 g_globalAmbientColor;
    vec3 g_sunColor;
    vec3 g_sunDirection;
};

layout(std140) uniform PSPerView {
    vec3 g_eyePosition;
};

layout(std140) uniform PSPerMaterial {
    vec3 g_ambientColor;
    uint g_hasTexture;
    vec4 g_diffuseColor;
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

    vec3 n = normalize(Normal);
    vec3 l = g_sunDirection;
    vec3 ambientColor = g_ambientColor * g_globalAmbientColor;
    vec3 directColor = g_sunColor * max(dot(n, l), 0.0);
    vec3 diffuseReflection = (ambientColor + directColor) * diffuseColor.rgb;

    vec3 v = normalize(g_eyePosition - WorldPosition);
    vec3 h = normalize(v + l);
    float highlight = pow(max(dot(n, h), 0.0), g_shininess) * float(dot(n, l) > 0.0);
    vec3 specularReflection = g_sunColor * g_specularColor * highlight;
    vec3 surfaceColor = diffuseReflection + specularReflection + g_emissionColor;

    float fog = smoothstep(g_fogColorAndStart.w, g_fogEndAndStrength.x, WorldPosition.z);
    fog *= g_fogEndAndStrength.y;
    FragColor = vec4(mix(surfaceColor, g_fogColorAndStart.rgb, fog), diffuseColor.a);
}
