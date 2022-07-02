#version 460 core

layout(location = 0) out vec4 oFragColor;
layout(location = 1) out float oFragDepth;
layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec2 iFragTexCoord;

void main() {
    vec3 normal = normalize(iFragNormal);
    oFragColor = vec4(normal * 0.5 + 0.5,1.0);
    oFragDepth = gl_FragCoord.z;
}
