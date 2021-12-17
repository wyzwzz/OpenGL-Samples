#version 430 core

layout(location = 0) in vec3 VertexPos;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 TexCoord;

uniform mat4 CameraMVP;

layout(location = 0) out vec3 outVertexNormal;
layout(location = 1) out vec3 outVertexPos;

void main(){
    gl_Position = CameraMVP * vec4(VertexPos,1.f);
    outVertexPos = VertexPos;
    outVertexNormal = VertexNormal;
}