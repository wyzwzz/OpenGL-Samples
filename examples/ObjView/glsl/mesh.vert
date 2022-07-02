#version 460 core

layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 TexCoord;

layout(location = 0) out vec3 oPos;
layout(location = 1) out vec3 oNormal;
layout(location = 2) out vec2 oTexCoord;

uniform mat4 model;
uniform mat4 vp;

void main() {
    gl_Position = vp * model * vec4(VertexPos, 1.0);
    oNormal = vec3(model * vec4(VertexNormal,0.0));
    oTexCoord = TexCoord;
}
